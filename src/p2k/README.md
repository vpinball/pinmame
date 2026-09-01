# Pinball 2000

Midway's last pinball platform: a PC with Cyrix MediaGX employing a
CX5520 southbridge, a PC97317 Super-I/O and a "Prism" PCI card carrying the ADSP-2104 sound
hardware (for the latter see `docs/pin2k_sound.md`, including outstanding issues and quirks of it) and the link to the pinball power driver board.

For table authors just interested in switch/coil/lamp mappings, go directly to `src/wpc/p2k_names.h`.

Every version of the P2K games is a driver of its own. The parent is the last one Midway shipped, not
the newest - the unofficial/aftermarket updates that carry on past it are clones of it, the way PinMAME handles
MODs elsewhere:

| Driver | Game | | |
|---|---|---|---|
| `rfm_260` | Pinball 2000: Revenge From Mars | 08/2024 | myPinballs; needs the opto expansion (v2.6+ per the kit) |
| `rfm_250` | | 12/2022 | myPinballs |
| `rfm_224r3` | | 01/2022 | myPinballs, also released as 2.42; the 29 Jan build, adds a Lyman Sheats tribute |
| `rfm_224r2` | | 01/2022 | the 13 Jan build; fixes an extra ball being awarded twice |
| `rfm_224r1` | | 12/2021 | the 19 Dec build, the initial 2.24 |
| `rfm_223` | | 04/2021 | myPinballs, also released as 2.40 |
| `rfm_222` | | 06/2020 | myPinballs, also released as 2.30 |
| `rfm_221` | | 04/2020 | myPinballs; last on 1.91's sound flash, 2.22 goes back to stock |
| `rfm_220` | | 10/2019 | myPinballs |
| `rfm_210r4` | | 04/2019 | the 11 Apr build, the 2.10 release; shaker/knocker support |
| `rfm_210r3` | | 04/2019 | the 06 Apr build |
| `rfm_210r2` | | 01/2019 | the 24 Jan build |
| `rfm_210r1` | | 01/2019 | the 19 Jan build. No package of its own - it is the unprefixed copy left in r2's archive - but its boot data is stamped like any other, so it is dated. r1-r3 share an im_flsh0 and a sound flash that r4 does not use |
| `rfm_200` | | 12/2018 | myPinballs' first; still XINA 1.22, keeps 1.91's sound flash, no shaker/knocker yet |
| `rfm_195r2` | | 03/2018 | the 29 Mar build, the 1.95 that circulated; German re-translation, used by nothing later - not the newest 1.x |
| `rfm_195r1` | | 03/2018 | the 27 Mar build |
| `rfm_191` | | 05/2018 | hemtoni; newest of the three numbers, added sounds - only 2.00/2.10 r4 kept them |
| `rfm_190r3` | | 03/2018 | the 29 Mar build - 1.90 repackaged, built 2h47m after 1.95 r2 and sharing its im_flsh0. This is what used to be plain `rfm_190` |
| `rfm_190r2` | | 11/2017 | the 22 Nov build, a next-day respin |
| `rfm_190r1` | | 11/2017 | the 21 Nov build, the original; drops 1.80's 8 MB requirement; its text runs through all of 2.x |
| `rfm_121` | | 05/2016 | **`GAME_NOT_WORKING`** - the only P2K set that is. hemtoni; numbered 1.21 but built 2016, and paired with `swep1_200h` - the only two on XINA "1.22 - Tue Jan 06 18:15:21 2016". Its `game.rom` carries a GRUB2 Multiboot header where every other image has a build date, and it drops into the Fatal monitor right after the loader starts it |
| `rfm_180` | | 04/2006 | Tom Uban, EPC 2006 tournament build; needs 8 MB |
| `rfm_170` | | 02/2006 | Tom Uban; "This version includes JTS". Still on 1.60's XINA 1.19 - 1.80 is where 1.21 and the 8 MB arrive |
| **`rfm_160`** | **parent** | 09/2003 | last official, XINA 1.19 |
| `rfm_150` | | 07/2000 | |
| `rfm_140` | | 01/2000 | |
| `rfm_130` | | 11/1999 | XINA 1.17, which 1.40 keeps |
| `rfm_120` | | 06/1999 | XINA 1.12 - so subject to the coin-door sound fault below |
| `rfm_087` | | 04/1999 | pre-production, XINA 1.04 |
| `rfm_086` | | 04/1999 | pre-production; linked *after* 0.87 - the pairs are parallel variants, not a sequence |
| `rfm_085` | | 04/1999 | pre-production, XINA 1.04 |
| `rfm_084` | | 04/1999 | pre-production; linked *after* 0.85. Within each pair (0.70/0.71, 0.84/0.85, 0.86/0.87) exactly two functions differ - both `award_switch()`, whose jump table has 36 cases in the odd build and 34 in the even, so the odd variant's switch enum has two extra members. Shipped 1.20/1.60 have 34, so the even builds are the line that shipped |
| `rfm_080` | | 03/1999 | XINA 1.04 - likewise |
| `rfm_071` | | 03/1999 | pre-production, XINA 1.02; built 26 minutes after 0.70 and unknown to the revision history |
| `rfm_070` | | 03/1999 | pre-production, XINA 1.02. These six share a sound flash of their own, and are the only pre-production sets with an update package rather than a Prism-ROM fallback |
| `rfm_010` | | 01/1999 | no ROMs of its own - `rfm`'s Prism chips with the update package left out, so the loader starts the fallback copy. Old XINA, so likewise |
| `swep1_210` | Pinball 2000: Star Wars Episode I | 10/2025 | myPinballs; adds a topper (driver 44) |
| `swep1_201` | | 05/2025 | myPinballs |
| `swep1_200m` | | 04/2025 | myPinballs; shaker/knocker support (drivers 42/43). The `m` separates it from hemtoni's unrelated 2.00 below. This line's code descends from Uban's `swep1_160`, not from hemtoni's 1.6x - see the note in `src/wpc/p2k.c` |
| `swep1_166r2` | | 04/2022 | the 03 Apr build. XINA 1.31, adds shaker support - on driver 5, where myPinballs' 2.x uses 43 and puts an auto plunger on 5 instead. Its changelog documents the whole version history |
| `swep1_166r1` | | 11/2021 | the 14 Nov build, which is the date the changelog gives the version; shares r2's im_flsh0 and symbol table. Runs, but prints two NonFatals about duplicate resource IDs that r2 does not - it is the beta its label says it is |
| `swep1_165r2` | | 04/2021 | XINA 1.31 and an im_flsh0 of its own; the changelog's "1.65 (03/2021)" |
| `swep1_165r1` | | 02/2018 | still XINA 1.22 and the 1.6x-era im_flsh0; the changelog does not record this build |
| `swep1_200h` | | 02/2016 | hemtoni; calls itself 2.00 nine years before myPinballs' unrelated `swep1_200m` above, hence the suffix. Nothing was built on the number - the line went back to 1.65 in 2018. Pairs with `rfm_121` |
| `swep1_160` | | 04/2006 | Tom Uban, and the Episode I counterpart of RFM 1.80 - their boot data was built 52 seconds apart, both on XINA 1.21. Both unofficial Episode I lines, hemtoni's and myPinballs', branch from here rather than from official 1.50 |
| **`swep1_150`** | **parent** | 09/2003 | last official, XINA 1.19 |
| `swep1_140` | | 07/2000 | |
| `swep1_130` | | 09/1999 | XINA 1.16 |
| `swep1_120` | | 09/1999 | XINA 1.16, which 1.30 keeps - the two were built five days apart |
| `swep1_110` | | 09/1999 | XINA 1.15; first official update |
| `swep1_100` | | 07/1999 | XINA 1.13; first production release |
| `swep1_040` | | 06/1999 | XINA 1.12; no ROMs of its own, same as `rfm_010`, and likewise subject to the coin-door sound fault |

Every set (bar the weird `rfm_121` unofficial one) boots, renders, takes coins, starts a game, drives lamps and coils, responds to the
flippers and plays sound - the pre-production ones included.

All of them were also checked the following way, which is worth repeating for any set added later - it
catches a mispaired or mis-sized update file immediately, because the loader validates each of the
four components against the boot data's own table:

```
P2K_PROGRESS=20000000 ./pinmame <set>
```

A healthy set prints four `[ VALIDATING UPDATE ... ]` lines, then `[ STARTING UPDATE GAME CODE ]`,
then the XINA banner - and that banner is an independent check on the set, since it has to match the
version the `game.rom` names.

Note that a first boot on a blank CMOS may also print *"CMOS Prologue Checksum Invalid"* and *"CMOS Manager
has wiped NVRAM at 0x11002400"* on the games whose console talks - the
shipped production builds seem not to. That is not a fault and not something the seed causes: it is a
machine being switched on for the first time and building the CMOS it does not have yet, which
is what real hardware does too. Boot the same set again with its `.nv` in place and the messages
are gone, so settings do persist. Only delete the NVRAM between runs if a fresh machine is the
point - otherwise every run looks like a first power-up.

Two things follow for sets that solely feature the Prism ROMs (i.e. prototype versions).
Usually, games employ an 8 MB update flash (the actual gamecode, etc) that the loader validates and jumps into.
The prototype games solely live in the Prism ROMs (with a fallback/older gamecode) and the boot loader takes its other branch,
printing `[ STARTING GAME CODE ]` where most other sets print `[ STARTING UPDATE GAME CODE ]`.
That branch is in every boot ROM seen, V3.2 through V3.6.

- Thus, the update flash initialises to `0xff`, not `0`, because erased flash reads all-ones. A set
  that carries an update region overwrites every byte and never sees the fill; a machine without
  one boots against it, and then it is the whole question.
- A set with **no** update region is no longer reported as an error. A region that is present but
  short still is - that set is damaged. See `set_nvram_updates()` and `p2k_pinmame.cpp`.

## Prototypes

### Wizard Blocks

Wizard Blocks is the first Pinball 2000 title that never shipped.
It was in its early stage, with rudimentary software and early whitewood playfield.
There seem to be dumps and aftermarket versions floating around, but none does appear in PinMAME so far.

### Playboy

Not much is known about that one, except that it was in an even earlier stage than Wizard Blocks.
Some collector is the last known person to own it, but there may also be more.
Also unknown is, if there exists a ROM/Flash dump somewhere already.

## Every machine carries a second, older game in its Prism ROMs

`u100`/`u101` hold a fallback copy of the game alongside the system data, and the update flash
overrides it. `P2K_NO_UPDATE=1` hides the update flash, and then the loader validates and starts
that copy instead. Each copy carries its own version in the boot-data header at bank 0 offset `0x8040`, as a
game id and a major/minor pair, formatted for display through `"Software version: %d.%d"`:

| Prism pair | set | game id | fallback version | bootstrap loader |
|---|---|---|---|---|---|
| RFM r1 | `rfm_010` | 50070 | 0.1 | V3.2, Jan 26 1999 |
| RFM r2(?) (`rfm_u100r2`/`rfm_u101r2`) | `rfm_080` | 50070 | 0.80 | V3.4, Apr 1 1999 |
| Episode I | `swep1_040` | 50069 | 0.40 | V3.5, Apr-Jun 1999 |

Each has a set of its own so it is addressable without the knob. Only `rfm_080` needs files of
its own; `rfm_010` and `swep1_040` are their parent's Prism chips with the update package left
out, so we load them straight from the parent set.

**The later loaders check that the update belongs to the game; V3.2 does not yet.** Put an update for
another game id on a card and V3.5 (Episode I) prints `[ UPDATE MISMATCH ]`
and falls through to the non-update path, starting their own ROM copy instead. V3.2 (RFM r1) has no
such check at all - Episode I 1.50's update runs on the RFM card, four components validated, XINA
1.19, no Fatals. Only worth knowing if you go looking: it means a foreign update boots on exactly
one card and is refused by every card new enough to look.

`rfm_080`/r2 has a set of its own, filed as a clone of `rfm_160` like every other version - r2
bank 0, no update region. Only `u100`/`u101` are actually rev. 2 - the MAME set carries no other
r2 file - but the set is probably complete rather than a guess: diffing the two revisions of that
pair, they differ only below `0x2fd3c0` of the interleaved bank, which is the loader and the
fallback game image, and the 13 MB of asset data above it is byte-identical. The revision changed
code and left the art alone, so carrying the r1 banks 1-3 over follows the same evidence. The
sound flash and sample chips are on the sound board, not the Prism card.

Adding it did force one real improvement. The PCI bring-up patch sits at `0x461b` in r2 and
`0x419a` in the r1 pair, and the two are the *same game* - no prefix can tell them apart. So
`set_prism_roms()` now finds the site by its five-byte signature `b8 f9 ff ff ff` rather than by
game name. That is exact: each of the four images matches at precisely one of the four known
offsets, and every other occurrence in that image sits above `0xc0000`. It also matters for
safety - r2 holds a `call` whose displacement begins where the r1 pair holds this immediate,
so patching by game there would have redirected the call.

## 1.95 is not the newest RFM 1.x

The version numbers of hemtoni's aftermarket RFM sets do not run in the order they were built. His changelog dates
them **1.90 (21 November 2017), 1.95 (29 March 2018), 1.91 (31 May 2018)** - so 1.95 is the middle
one, not the newest, and 1.91 is the last. The changelog documents 1.9 and 1.91 and never mentions
1.95 at all.

The packages muddle this, which is worth knowing before dating them from the files: all three were
put up in 2018, so 1.90's package is stamped March 2018 even though the version is from November
2017, and all three `game.rom`s carry that same November 2017 build stamp because 1.95 and 1.91
inherit 1.90's rather than restamping.

**1.90 is the one that lasted, and 1.95 and 1.91 are both branches off it.** Ask what the versions
after them kept. Of the 64 strings 1.90 has and 1.95 does not, and the 52 the other way round, 1.91
has 48 and 2 - and so does every myPinballs release, 2.10 through 2.60 scoring 47-48 and 0-2. 1.90's
German is what the whole line after it carries; 1.95's re-translation appears in nothing later.
(Official 1.60 scores 3 and 9 on the same test, so 1.95 was pulling the wording back towards the
factory text rather than away from it.)

1.91's own change went the same way one release later. It is 1.90 plus new sounds and little else -
it adds 12 strings over 1.90, eleven of them jump table bytes and the twelfth its XINA banner,
because the sounds live in the sound flash and not in `game.rom`. Only 2.00 and 2.10 ship that
flash again; 2.22 onwards are back to the stock one. So 1.91 reads as a variant that added sounds and was then
dropped too - it just got picked up once, which 1.95 never was.

The 1.90/1.95 text difference itself runs to roughly 40 display strings, along these lines:

| | 1.90 and 1.91 | 1.95 |
|---|---|---|
| the button name | `ACTION-TASTEN bewegen Fadenkreuz` | `AKTIONSTASTEN bewegen Fadenkreuz` |
| credits, pluralised | `%d KREDITE VERGEBEN` | `%d KREDITS VERGEBEN` |
| UFO or ship | `Alle UFOs zerstoert` | `Alle Schiffe zerstoert` |
| killing things | `%u von %u abgeknallt` | `%u von %u abgeschossen` |
| | `%u von %u rausgeschmissen` | `%u von %u zerstoert` |
| | `Martian%q1//s/% gekillt: %u` | `Martian%q1//s/% umgebracht: %u` |
| phrasing | `Auf UFO und Martians schiessen!` | `Auf UFO schiessen, dann die Martians!` |
| | `waehrend Ball 1 rollt` | `waehrend 1. Kugel rollt` |
| | `TREFFE RAMPE` | `SCHIESSE RAMPE` |

1.95 reads as the more literal and formal pass, 1.90 as the looser and more idiomatic one.

1.90 also translates the tournament screens, which 1.95 leaves in English - `Karte einlesen zum
Spielen.`, `Keine Liga gefunden.`, `Keine qualifizierten Spieler.`. That is a translation
difference and not a missing feature: 1.80, 1.90, 1.91, 1.95 and 2.60 all carry the card-swipe
tournament client and its English strings. 2.60 has the German ones too, so myPinballs picked up
that translation along with the code.

## Where the original PR came from

The initial machine emulation was based on a port of the MAME driver by **erikieNL** (`erikieNL/mame`, branch `pinball2k`, MAME
0.239), which again was built on R. Belmont's skeleton and Ville Linde's `mediagx.c`. The x86 CPU, the
chipset devices and the handler logic are that driver's; additional info was derived from Encore by reading
its published source; what was new to the PR, was the plumbing that lets MAME 0.239 code live inside a fork of MAME 0.76.

The DCS sound board is **not** ported - it is PinMAME's own Pin2K board in `src/wpc/wmssnd.c`
(`docs/pin2k_sound.md`), which this driver drives as its host.

Since then, a lot of code has been rewritten, extended, fixed, generalized, and optimized.
What remains almost original (see **PINMAME** markers in the source though) are the parts that live in `src/p2k/mame`.

These imported MAME files keep their original copyright and license headers (MAME is BSD-3-Clause) and
name the version they came from.

## A warning about speed

Pinball 2000 is a 233 MHz PC and this emulates the whole thing - protected mode, paging, the x87
FPU, the PCI bus, blitter operations, the display controller. The CPU is clocked down, though no longer as far as MAME's
driver takes it: 233/3, about 77.7 MHz, set in `src/wpc/p2k.c` and mirrored in `p2k_pinmame.cpp`.
Note that all video/gfx operations (a bottleneck on the real machine) are close to free to this emulated CPU though,
so this lower clock rate does not match what a real MediaGX CPU would be capable of at the same clock rate.

So in practice, games run at a frame rate that should match the original, and the required emulation system must
not be high-end anymore (compared to the MAME driver).
Note that the workload is single-threaded, so only single-core performance matters.

Measured shares in a running game (Windows, and a bit outdated): the MediaGX 70-90 %, the DCS sound board's ADSP-2100 around 5 %.
Note that the low ADSP percentage is due to implemented idle skips (see below for potential caveats); most of that sound
board share turned out to be the DSP waiting rather than working, so treat it as an upper bound.

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

Visual Studio: Best is to add `P2K_DEBUG=1` to the project's preprocessor definitions. `src/p2k/p2k_debug.h`
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

A set usually holds three things: The sound board's flash and its two sample chips; the MediaGX side's
eight Prism ROMs, `u100`-`u107`, which the loader interleaves in pairs into four 16 MB banks; and
the 8 MB update flash, which is what makes a version a version. The update flash is not a real dump -
it is the four files of an update package (`bootdata`, `im_flsh0`, `game`, `symbols`) laid
out back to back at the offsets the boot ROM looks for them at. Everything else is shared, so a clone's zip contains only its own four update files -
bar some aftermarket RFM and SWEP1 sets which bring their own sound flash too, and so hold five.

#### Getting the four files out of an update package

Updates ship as `pin2000_<game>_<ver>_<date>_B_10000000.exe`, where `<game>` is the Williams part
number - `50070` for Revenge From Mars, `50069` for Star Wars Episode I - and `<ver>` is the
version with the dot dropped and padded to four digits, so `0180` is 1.80. Despite the extension
they are PKZIP self-extracting archives (`PKSFX CLI for Windows 95/NT`, deflate), so any zip tool
opens them, `python -m zipfile -e <pkg>.exe <dir>` included. Inside is a `<game>/` directory with
the four files above plus two the machine does not take from the package:

* `_sf.rom` - 1 MiB in every package, official and unofficial alike. This one is *not* inert: it is
  the DCS sound board's flash, the same image the sets declare as `rfm_28f800.rom` /
  `swe1_28f800.rom`. In 34 of the 49 packages behind a set here it is byte-identical to those, which
  is why those sets can share one copy. The other 15 ship something else, but that is only **four**
  distinct images:
  * the six RFM 0.7x/0.8x pre-production packages carry one of their own, 89.6% different from the
    shipped chip - the audio was still being cut when they were made;
  * RFM 1.91's, which 2.00, 2.10 r4, 2.20 and 2.21 ship again under their own version's name, so
    those five sets all declare **1.91's name** rather than their own - one image is not worth five
    ROM entries and eight more megabytes;
  * RFM 2.10 r1-r3's, which is 1.91's with exactly two bytes changed, at `0x0122bc` and `0x016200`
    - out in the sample data, not a header and not a checksum;
  * SWEP1 2.10's.

  The note next to the ROM definitions in `src/wpc/p2k.c` has the rest.
* Text and metadata, in some packages only, none of it loaded:
  * `gamelist.txt` - in nearly every package, three variants by era (64, 84 and 135 bytes).
  * `changelog.txt` and `xina_changelog.txt` - **only** in the hemtoni Episode I 1.65/1.66 archives,
    and the single best documentation of that line. Note each archive carries `changelog.txt`
    *twice*, at the root and inside `50069/`, and the two differ: the inner copy is the previous
    release's, left behind. Always read the root one. Its history runs 1.66, 1.65 (24 MAR 2021,
    XINA 1.31), 1.6 (23 APR 2006, XINA 1.21), 1.5 - dates and XINAs matching the sets here - and it
    records neither `swep1_165r1` nor `swep1_200h`.
  * `_notes/dwsync.xml` - Dreamweaver site-sync metadata in twelve myPinballs RFM archives, left in
    by accident. It names the author's two upload targets, `10.0.1.230//root/pinbox/update/rfm_15/`
    and `10.0.1.47//root/pinbox/update/rfm_15/`, and carries real file mtimes in the Mac 1904 epoch.
    Like the stale ROM copies it is one release behind the archive it ships in, so it is no use for
    dating a set - but it does show the stock sound flash still carrying its original **1999-04-13
    16:27** mtime in 2024, and 1.91's carrying 2018-05-26.
* `_pubboot.rom` - absent from the RFM 0.7x/0.8x packages and present in every other one, in four
  images that go strictly by age rather than by game: SWEP1 1.00 has its own (32256 bytes), SWEP1
  1.10/1.20/1.30 share a second (32256), RFM 1.20 has a third (31744), and from SWEP1 1.40 and RFM
  1.30 onwards every package carries the same fourth one (32768) - official and unofficial alike,
  through to 2.60. Nothing loads it here and Encore's loader ignores it too. The name is the PUB
  card, the loader medium the last official updates were carried on before anyone repackaged them
  as serial updates.

The four that matter go into the set's zip under the names they already have, and `ROM_START` in
`src/wpc/p2k.c` places them; nothing has to be concatenated by hand. `bootdata` is 32 KiB in every
package known, which is why `P2K_UPDATE` can hardcode `0x8000` as the start of `im_flsh0` and take
only the other three lengths - and the three lengths are how the loader finds the parts, since it
matches by filename suffix and has no size table to consult.

CMOS and the PLX EEPROM persist through PinMAME's normal NVRAM file. The update flash deliberately
does not: it belongs to the set, and an 8 MB NVRAM file would take precedence over it - a machine
would keep booting the version it was first started with even after selecting another driver.

#### Dating new sets: four dates, and only one is a build time

Every set here can be dated four different ways and they routinely disagree. In order of how much
they are worth:

1. **The boot data's build stamp**, an ASCII timestamp at offset 0 of `bootdata.rom`. This is the
   real one, and it is what the set years in the table above come from.
2. **The package filename**, `pin2000_<game>_<ver>_MMDDYYYY_...`. This is when somebody *packaged*
   it, which can be years after the build - `swep1_160` is packaged `02012013` and built
   2006-04-23, off by 2476 days, and the `07252000` Episode I 1.50 package holds files built
   2003-09-22. Never date a set by its filename.
3. **The game image's stamp**, at offset 0x0c of `game.rom`. Useless for dating the aftermarket
   builds, because they do not restamp: all five RFM 1.9x builds carry `Tue Nov 21 11:33:08 2017`,
   and the 06 Apr 2019 RFM 2.10 carries the 10 Jan one. It *is* useful for something else, below.
4. **The version the machine prints.** Lossy: the header stores major and minor separately and
   `"Software version: %d.%d"` prints the minor as a plain number, so 1.90, 1.91 and 1.95 all
   display `1.9`, 1.60 displays `1.6`, and `rfm_010` displays `0.1`.

**The `game.rom` clock is not a clock.** In every official release the mm:ss of that stamp is the
version number (below), so it cannot also be a build time - and in the pre-production sets it is
constant across builds days apart (`03:28` for both 0.7x, `06:24` for all four 0.8x) while the hour
and date move. Whatever writes that field, it is not `ctime()` in any of these images, so do not
read a minute-level time off it. The **boot data** stamp is the real clock: its mm:ss is scattered
with no pattern in official and unofficial sets alike, which is why the set years come from there.

**The version-in-the-timestamp convention.** Williams encoded the version number into the *time*
field of both stamps, as `<major>:<minor>` - so XINA 1.19's banner reads
`1.19 - Mon Sep 22 07:01:19 2003`, and official Episode I 1.40's game image reads
`Tue Jul 25 01:40:00 2000`. The date part is real; the clock part is not a clock. This splits the
whole collection cleanly and is worth knowing, because it is an independent check on who built a
given image:

* Every official XINA, 1.02 through 1.21, follows it. **No** XINA from 1.22 on does - the
  aftermarket kept 1.21's `01:21` for a while (1.22 and 1.31 both carry it) and then stopped
  bothering, which is also why a XINA version number alone does not identify a build: 1.22 appears
  with **four** different timestamps here - two of them a day apart in January 2016 - 1.31 with two, 1.36 with two.

  The XINA changelogs do not settle 1.22 either - they disagree with each other. The standalone
  `XINA_Revision_History.txt` dates it `Fri May 18 19:58:21 2018` and describes it as a "TCP-Port
  bug fix"; the `xina_changelog.txt` inside the Episode I 1.66 package dates the same version
  `Mon Nov 16 21:58:21 2017` and calls it an "Ephemeral TCP-Port bug fix". Same version number,
  same fix, eighteen months apart, and neither date matches the `Mon Nov 20` stamp the 1.9x images
  actually carry. Treat "XINA 1.22" as a label rather than a build, and go by the timestamp.
* Every official game image follows it - RFM 1.20 to 1.60, Episode I 1.00 to 1.50 - **and so do
  RFM 1.70, RFM 1.80 and Episode I 1.60**, which are Tom Uban's. Nothing else does: not one
  aftermarket build, and not the 0.7x/0.8x pre-production ones, which pre-date the convention.

  That last point is worth its weight: those three carry on using Williams' own internal build
  convention, which no other unofficial build does. It is independent support for the attribution
  to the original programmers, alongside RFM 1.80 and SWEP1 1.60's boot data being built 52 seconds
  apart.

#### Some versions circulate under two archive names

At least three packages exist under a second filename with byte-identical contents, so a collection holding
both looks like it has more versions than it does. Checked file by file against the
[Encore-Pinball2000](https://github.com/ThomazPom/Encore-Pinball2000) `updates/` tree:

| this name | is the same four files as |
|---|---|
| `pin2000_50070_0180_09222003_B_10000000` | `..._0180_04232006_...` - which is where 1.80's misleading 09/03 date comes from |
| `pin2000_50070_0190_03292018_B_10000000` | `..._0190_03312018_...` (`rfm_190r3`) |
| `pin2000_50070_0191_05302018_B_10000000` | `..._0191_05312018_...` (`rfm_191`) |

#### The r2 boot ROMs

The MAME/Encore sets also carry `rfm_u100r2.rom` / `rfm_u101r2.rom`, the second(?) revision of Revenge From
Mars's bank-0 Prism pair.

* **The name is MAME's**, from its `rfmpbr2` set - "Pinball 2000: Revenge From Mars (rev. 2)", a
  clone of `rfmpb` that differs in `u100`/`u101` and nothing else, with both dated 1999. It is a
  factory revision of the card, not a later one, so none of the 2.x updates can be asking for it.
  MAME needs a set for it because its P2K driver loads no update flash (yet): there, the Prism pair is
  the only thing that can tell two machines apart. And `u100`/`u101` are not the sole home of the
  game code either - they hold a fallback copy plus system data, which the update flash overrides.
  It is unknown though so far if its really rev. 2, or an even later rev. Only rev. 1 is confirmed.
* Same 8 MB each, byte-identical to the stock pair from `0x17e9e0` upward and rewritten below it:
  about 15% of each chip, concentrated in the first 1.5 MB.
* **The driver needed work, not just a `ROM_START`.** It patched bank 0 on the way in
  (`p2k_state::set_prism_roms`). `0x191` holds the same `retf` (`cb`) in both revisions, so that
  one carries over - Episode I has `cb` there too. `0x419a` does not carry over, and note it is the
  *immediate*, not the instruction: the stock pair has `b8 f9 ff ff ff`, `mov eax,0FFFFFFF9h`,
  starting a byte earlier at `0x4199`, and the four immediate bytes are forced to 1 to make a
  failing check report success. r2 has `e8 9e 27 00 00` at that same `0x4199` - a near `call`,
  `+0x279e` - so those four bytes are its displacement, and poking 1 into them sends the call
  somewhere arbitrary.
* **But the check has now since then be identified, and r2's address with it.** The patched instruction is
  the "already initialised" early exit of the MediaGX PCI bring-up - the routine that walks device
  numbers 0..0x14 for vendor `0x1078` (Cyrix), picks out the host bridge and the Cx5510/Cx5520 ISA
  bridge, programs their BARs and returns 1. The patch makes that early exit return 1 as well,
  instead of -7, so a second call is told the chipset is ready rather than returning an error the
  caller has no handler for. It is not a self-test or a protection check. The same routine is in
  the r2 image, moved: function at `0x4608` instead of `0x4184`, guard flag at `0x877c0`, and the
  immediate to patch at **`0x461b`** instead of `0x419a`. So an r2 set needs one address changed,
  not fresh reverse engineering. `p2k_state::set_prism_roms` carries the disassembly and the byte
  patterns for re-finding it in any image.

##### To investigate: when did rev. 2 ship, and with which version?

Two things need establishing:

1. **When was rev. 2 released to the public?** MAME dates both revisions 1999 and says no more.
2. **Which official shipping version of the game was the first to leave the factory on it?** (speculation: 0.80 or 0.90)

Rev. 2 arrived partway through the production run (as its older than rev. 1), so versions from that point on shipped on
rev. 2 cards and the earlier ones on rev. 1 (most likely) - and each set should declare whichever pair its
version was actually sold with, rev. 1 or rev. 2. Today every set declares rev. 1, which is an
unchecked assumption rather than a finding.

There is now some evidence on the first of those, and it is dated rather than inferred. Each Prism
pair carries its bootstrap loader with a build timestamp: stock RFM has **V3.2, Tue Jan 26 1999**
and r2 has **V3.4, Thu Apr 01 1999**. The fallback game copies agree in direction - `0.1` on the
r1 pair against `0.80` on r2. So r2 is not a contemporaneous alternative source for the same
machines; it is roughly two months later and most of the pre-release version range further on,
which is what a revision arriving partway through the run looks like. For scale, Episode I's pair
is V3.5 - so the V3.x loader line kept moving through
1999 across all three games.

That still does not answer the second question. Nothing here says which *shipping* version of the
game was the first to leave the factory on an r2 card, and the loader date only bounds when the
card could have been burned, not when it went out. Also it could well be that on-stock rev. 1 chips
were first used up until switching (as these ROMs were costly), or it was a wild mixture in practice,
depending on production line, etc.

Answering it also disposes of the "which version would an r2 set be a clone of" question above: the
r2 pair would simply belong to the versions it shipped under, and would stop needing a set of its
own. Note that this only decides which pair each set *declares* - it does not change how the game
plays, since the update flash overrides the game code either way. What it changes is which boot
image is authentic for a given version, and - because the `0x419a` patch does not carry over -
whether such a set boots here at all.

Another open question is, if there were actually other masked ROMs inbetween rev. 1 and rev. 2.
This could be answered by finding a full internal changelog, similar to what showed up for SWEP1
(where 2 masked ROM releases, one internal, one for production, were listed in).

The hashes are in the ROM section of `src/wpc/p2k.c` so the files stay identifiable if someone picks this up.

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
| **B** | Place balls in trough |

**Close the coin door first.** The machine starts with it open, says so on screen, and keeps high
voltage off until the door *closes* - the firmware raises the high-voltage relay on that edge and
there is no separate interlock input. Press END and the warning goes.

### Playing it standalone

A real Pinball 2000 gets its switch feedback from a playfield. Under VPinMAME/libPinMAME the externally simulated table provides
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
  `coinb<n>` drives one bit of the coin register by itself, `coin` being bit 1 - the board does
  not put the three slots in the low bits, and this is how a position is checked rather than assumed.
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

Switch and coil names come out of each game's own device tables. Lamp names come from the
operations manuals instead, the lamp table's layout in the image never having been worked out, and
the manual's cell numbering maps to PinMAME's matrix through the board's two row banks - see the
header of `src/wpc/p2k_names.h`, which records how that was measured against the games' own lamp
tests. The extraction tool lives with the port's working notes.

### The I/O update cycle

The board is driven from a frame interrupt (`MDRV_CPU_VBLANK_INT`), not from the video renderer.
That matters: `PINMAME_VIDEO_UPDATE` is reached through `draw_screen()`, which `src/mame.c` only
calls when `osd_skip_this_frame()` says no, so while the I/O lived there a skipped frame meant the
machine received no switches and nobody read its coils. `wpc.c` and `se.c` both use a CPU interrupt
for the same job, and it cannot be skipped.

**Switches are read live.** The board indexes `coreGlobals.swMatrix` at the moment the game strobes
a column, rather than a copy taken once a frame, so a table or a keypress lands immediately. This is
what the WPC and Whitestar read handlers do.

**Outputs are accumulated, not sampled.** This is the part that is easy to get wrong, and the
measurements are worth keeping because they are what the design rests on. Logging every driver
register write on `rfm_160`:

| what | measured |
|---|---|
| driver board service tick | every **0.92 ms** (~1080 Hz) |
| lamp column strobe | 8 one-hot columns plus a `0x00` blank; each column re-strobed at **136 Hz** |
| Left/Right Martian (drivers 1-2) | on 3 writes, off 3 writes - **50% duty at ~180 Hz**, in ~34 ms bursts |
| Jet Exit Post (driver 3) | on 3 writes, off 9 - **25% duty**, held for over a second |
| gates (drivers 4-5) | one steady run of 1.2 s and 2.0 s, no modulation |

These coils have no separate holding winding, so the software chops the main one - the same reason
`wpc.c` gives for its own smoothing. Reading the latch once per frame samples that square wave at
60 Hz, which reports whichever phase it happens to land on and drops any pulse that falls between
two samples. It was visible: `swep1_150`'s neon tube toggled on and off every two or three frames,
101 transitions in one attract run, of which 56 were three frames apart or less.

So every write ORs into an accumulator in `p2k_state`, and `pull_outputs()` hands over the window
and starts a new one. Lamps clear to nothing, so a column the game stops strobing goes dark;
solenoids reseed with the current level, so an output still held stays on and only a released one
falls away. That asymmetry is `se.c`'s.

`P2K_SOLSMOOTH` and `P2K_LAMPSMOOTH` in `src/wpc/p2k.c` then hold each published result for a
couple of frames, as `WPC_SOLSMOOTH` and `SE_SOLSMOOTH` do. Both are 2 here rather than WPC's 4 for
coils: the window only has to outlast the gap between two pulses of one output, the longest chop
period measured is about 11 ms, and 33 ms gives a threefold margin where 16.7 ms would not. Flipper
coils are published every frame regardless - `se.c` treats its `flipsol` the same way - because they
are the one output where a player would feel the extra hold.

After the change the neon settles to 3 transitions instead of 101, no driver on either game shows a
gap of three frames or less, and the genuine attract blink on driver 29 keeps its 16-frame period -
which is the check that matters, because over-smoothing would have flattened it.

What this does **not** do is model duty cycle: 25% and 100% both read "on". That is the PWM work
still outstanding, and the note above `p2k_getSol()` in `src/wpc/p2k.c` says what it would take.

### Modulated outputs (PWM)

Coils, flashers and the lamp matrix are all part of PinMAME's PWM integration.

The board drives its outputs in shades, not on and off. Logging every driver register write on
`rfm_160` during a game:

| output | measured drive |
|---|---|
| coils (Left/Right Martian) | chopped **180 Hz at 50% duty**, in ~34 ms bursts |
| Jet Exit Post | held at **25% duty** for over a second |
| flashers (17, 22, 23, 25-28) | solid 15-16 ms pulses, often in trains ~10 ms apart - **~60% duty at 40 Hz** |
| lamps, 123 of 128 | plain on/off |
| lamps, 5 of 128 | **50% duty at 34 Hz** - a real half-brightness |

These coils have no separate holding winding, so the software chops the main one - the same reason
`wpc.c` gives for its own smoothing. The binary path reports all of this as simply "on".

With PWM on, `p2k_solPwm()` reports each edge from the register write and the core times it. A #89
arch flasher then reads 0.06, 0.19, 0.59, 0.71, 1.02 over six frames and decays over eight more,
and a short pulse on a #906 peaks near 0.17 - it never gets bright, which is what a 15 ms pulse
does to a filament. Coils stay binary under `CORE_MODOUT_SOL_2_STATE`, which is right: the question
about a coil is whether it fired. Values above 1.0 are normal for a briefly over-driven filament
and `saturatedByte()` clamps them at the consumer.

Three things are worth knowing before extending this.

**The flasher list is not hand-written.** `wpc.c` spells out per game which solenoid carries a `#89`
and which a `#906`, hundreds of lines of it. Here the manuals' part numbers were already recorded
against every driver in `p2k_names.h`, so they were promoted from comments to a `P2K_DEV_` field and
`p2k_initPwm()` loops the table, which is data already checked against the machines' own test menus.
Revenge From Mars and Episode I are covered, 22 flashers between them.

**The binary publish and the integrator overlap, on purpose.** `CORE_MODOUT_SOL_2_STATE` sets
`fastOn`, which makes the integrator write `coreGlobals.solenoids`/`solenoids2` itself on every
edge - the same words the smoothing window publishes. `wpc.c` and `se.c` have the same overlap and
the more recent writer simply wins. Having the window stand aside instead was tried and is worse:
the accumulators then never reset, and drivers 37-48 go stale in a word that `p2k_getSol()` reads
for the binary path.

**`CORE_MODOUT_FORCE_ON` must not be used while `nLamps` is 0.** `core_update_pwm_outputs()` has a
fallback under that flag which rebuilds `coreGlobals.lampMatrix` from `physicOutputState` whenever
the updated range touches the lamps - and an empty lamp range still satisfies its bounds test, so
every lamp would be rewritten to off. The driver enables `CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS`
alone, and leaves the choice to the user rather than forcing it as `capcom.c` and `sam.c` do.

#### Trying it

`P2K_PWM=1` turns the integration on for a standalone run - nothing outside VPinMAME and libpinmame
ever sets `options.usemodsol`, so there is otherwise no way to exercise it. `P2K_PWMWATCH=1` then
prints each coil's integrated value as it changes:

```
[p2k pwm] frame 2257:  23 Left Arch Flasher            0.586
[p2k pwm] frame 2260:  23 Left Arch Flasher            1.020
[p2k pwm] frame 2268:  23 Left Arch Flasher            0.193
```

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
imported files are taken (mostly) **unchanged** and given the small part of the modern API they need:
address spaces, a device tree with timers and callbacks, and no-op stubs for save state and the
debugger. That is `shim/` - about 1400 lines against 28000 imported. The measured fact the approach
rests on is that MAME's i386 core needs almost nothing from MAME's core; the chipset devices are
what force the shim to grow.
Note that by now, the i386 has been slightly extended and some optimizations (mainly a fast memory read,
and various shortcuts (that all generalize though!) along with cache layout changes) have been added.

**The bus.** The machine owns its own address spaces rather than going through PinMAME's memory
system. Plain-memory ranges - main RAM, the framebuffer and its `0xc0000000` alias - are served
straight out of the buffer by the address space, skipping an indirect call and a chain of range
compares. That is worth about 20 %; `P2K_FASTBUS=0` turns it off.

**Video.** No DMD: the picture is the MediaGX framebuffer, 640x240 RGB555, exported through
PinMAME's normal video path as a `CORE_VIDEO` layout with a renderer - the same shape `byvidpin.c`
uses e.g. for Baby Pac-Man - which is what feeds libPinMAME and any frontend behind it. The lines are
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

**Sound.** The Prism card's window at `0x13000000` is the host side of the DCS board - a command
port, a status register and an echo register. The driver forwards those and does nothing else; the
sound emulation is `wmssnd.c`'s. What lives here is the *host* half of the protocol, which is the
part that was missing: the stream is driven by the game code on the PC.

**Interrupts.** The game OS's clock handler services the pinball driver board one register at a
time and previously took longer than a tick period, so a tick nests inside its own handler and the OS
complains. Suppressing the timer edge did not help - the request was already latched in the
interrupt controller. Holding the controller's line away from the CPU while the guest is inside its
clock handler does. `P2K_CLKINT_GATE=1` re-enables this, but by now it has been addressed differently.

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

**And most of the rest is waiting for the host**:
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

**Important Note**: SWEP1s intro (infamous scrolling text) does fail to read the 2nd sentence.
We need to check if this is due to the skip hacks above!

## Possible performance related extensions

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
* **Outdated numbers! : It bought +8 %, not 1.5x.** The parallelism is real (1.28x) but some 18 points of extra work
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

Still, the amount of performance bought must still be below 5 % overall, as the MediaGX still dominates
everything with the DSP idle skips in place.

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

* **A general profile of the whole emulator**, in host time rather than opcodes. Visual Studio's
  CPU Usage profiler has since answered the first question - the sound board is barely measurable
  now that both idle skips are in, and the MediaGX interpreter's `FETCH()` dominates. `perf` does
  the same on Linux, which has not been repeated since.
* **What the sound board costs in wall clock**, as a number rather than "barely measurable". The
  38 % at the top of this file is a pre-skip figure from a different machine under Linux, so it is
  an upper bound at best. If a cheaper route is wanted than an external profiler, `cpuexec.c`
  brackets every CPU's execute with `profiler_mark(PROFILER_CPU1 + cpunum)`; those marks compile
  away unless `MAME_DEBUG` is defined, and enabling just them gives per-CPU shares without the
  rest of the debugger.
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

The other half. PinMAME already ships `ext/asmjit`. With the MediaGX at 70-90 % of the work, Amdahl
caps, so on slow hardware it reaches real time only together with the threading or ADSP optimizations above.

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

* All platforms are built *and run*: all games boot and render under
  the `PinMAME` executable built from CMake, vcproj files and lists them.
* The picture VPinMAME hands a frontend is 641 pixels wide for a 640-pixel screen. That extra
  column is `core_findSize()`'s `+ 1` and every `CORE_VIDEO` game has it - Baby Pac-Man and Granny
  report 257 for a 256-pixel screen. It is left alone deliberately: changing it would move those
  games' output too.
* **Lamp fault detection: per-lamp works, the matrix test is unconfirmed.** Pinball 2000 lets an
  operator find a dead bulb from the test menu, by driving a lamp and reading it back. That
  readback is registers `0x10`/`0x11` on the power driver board - the 74LS240 the operations
  manual marks "Lamp Status", used for diagnostics only. This driver returned a constant `0x00`,
  so every lamp on both machines reported as short circuited. It now echoes the lamp row latches,
  modelling a playfield where every bulb is present, and **the per-lamp test passes on both games**.
  The sense (`P2K_LAMP_STATUS_INVERT`, 0 = echo un-inverted) was measured with `P2K_PDBWATCH`, not
  inferred: the test walks one row bit at a time and reads back after each, and the driven bit has
  to read back set.

  What is *not* confirmed is the whole-matrix test that runs at power-up (`lamp_powerup_tests`,
  filling `poweron_open_matrix`). Note before chasing it that `Test Lampe Matrix A` is that test's
  **progress title**, not a fault - the verdict is the separate `%d Lampen Probleme` line, and the
  column-level fault text is `Column %d%c` + `OPEN CIRCUIT`. Two things are known: the test-menu
  screen itself performs no reads at all and only walks columns with the rows clear, so it displays
  a stored result rather than measuring; and clearing NVRAM does not change what it shows. The
  game-side entry points are named in the packages' `symbols.rom` - `lamp_powerup_tests`,
  `diagnostics_is_lamp_bad`, `poweron_open_matrix`, `lamp_test_report_print`.

* **Four power driver board registers answer with constants**, and all four have now been watched
  rather than assumed. `P2K_PDBWATCH=02,0c,0d,0e,0f,12,13` across boot, attract and the test menu
  shows `0x02` (dip switches) and `0x0f` (switch-system, which carries zero cross) each read
  exactly once at startup, with the constants accepted; `0x0c`-`0x0e` written constantly and never
  read, being the solenoid registers. `0x12`/`0x13`, the fuse diagnostics, were reported here as
  never read at all - that was wrong, and only because the watch never walked into the service
  menu's fuse test, which is the one thing that asks for them. It reads them as a pair through
  `wms_pdb_fuse_status(unsigned char &, unsigned char &)`, gets `00`/`00` from the constants here,
  and draws every fuse green - so the constant is the healthy answer rather than an ignored one.
  So
  none of them is load-bearing as things stand. Zero cross in particular is not polled, so nothing
  is timed against it - that only becomes worth modelling if something starts reading `0x0f`
  repeatedly. The fuse pair only needs work if a blown fuse ever has to be shown, which nothing
  asks for; the XINA changelog adds that the test "checks for coin door closure" (1.01) and
  "holds state while the coin door is open" (1.02), so it is gated on the door rather than
  free-running.
* **SOLVED - the UART divisor latch clobbered the interrupt enable.** For a long time `swep1_200m`,
  `swep1_201` and `swep1_210` looked like they had a broken service menu: the background never
  repainted, the switch test came up blank, a coin could hang the machine and keys stopped
  registering in game. All of it was one fault, and it was in this driver.

  Ports `0x3f8`/`0x3f9` are two registers each, selected by DLAB in the line control register, and
  this driver honoured that for port 0 but not port 1. XINA's `tty_set_port_param()` sets DLAB and
  writes the 9600-baud divisor `0x000c`; the high byte, zero, landed on the interrupt enable and
  switched the console's transmit interrupt off for good, about 3.8 seconds in. The output queue
  then never drained, filled after ~35 seconds of ordinary messages, and `ttyputc` blocked forever
  on the semaphore guarding it with the executive behind it. Anything that printed more - a coin,
  the service menu - got there sooner, which is why those looked like triggers. Fixed in `port_w`.

  Three things from the hunt are worth keeping, because each cost hours:

  * **`*** Fatal: exec is hung` only reaches the log when `P2K_PROGRESS` is set** - the guest
    console is dumped by `report_progress()` and nothing else, so a run without it greps clean no
    matter what happened on screen. Always set it when the question is whether something hung.
  * **`P2K_PCTRAP` prints two different prefixes.** Arming says `[p2k pctrap]`, hits say
    **`[p2k pc]`**. Grepping for the wrong one had it written off as broken for a long stretch.
  * **Check hit counts against `P2K_PCTRAP_MAX`.** A trace that stops because the cap was reached
    looks exactly like one that stops because the events did, and reading it the wrong way sent
    this investigation after a non-existent fault in the display manager.

  The blitter and display side were exhaustively ruled out along the way - raster modes, vector
  mode, fills, controller programming, blit height, out-of-range sources, the pipeline registers,
  the colour key, the vertical interrupt, the line counter. None of it was at fault, and the
  measurements are in the git history if they are ever wanted again.

* **`DC_TIMING_CFG` bit 30 is a vertical blank status**, set during active display and clear while
  blanking, and it was missing. MAME's own MediaGX driver has it - `src/mame/atari/mediagx.cpp`,
  deriving it from `m_screen->vpos()` - and the `pinball2k.cpp` this port came from dropped it
  along with the screen device it needed, the same way `DC_V_LINE_CNT` was dropped. Without it the
  register read back exactly what was written, so anything polling for the edge waited for ever.
  The games do write it, `0x2804f` and `0x2806f`, so they know it is there. It does **not** fix the
  1.38 service menu, which is what prompted finding it.
* **The vertical timings are taken from the registers now, not assumed.** They used to be a
  hardcoded 525 lines at 60 Hz, the VGA 640x480 default, which describes the output after line
  doubling rather than what the display controller counts. The games programme `DC_V_TIMING_1` =
  `0x010400ef`, which is **240 active of 261 total** - and Encore's counter running 0..241 is the
  same figure reached independently. The line counter was therefore cycling over roughly twice as
  many lines as exist, on every set. `video_lines()` derives both from the register now, falling
  back to the old constants before the game has programmed anything.
* **The PCI watchdog is not modelled.** It is real hardware - XINA 1.02 changed the system so
  that "instead of turning it off, those operations which may cause it to expire will call
  `pci_watchdog_bone()` in order to put off the dog", and `pci_watchdog_pause/bone/test` are all
  in the games' symbol tables. Nothing here answers for it. That is harmless as things stand,
  because petting it is writes this driver absorbs and no set has been seen to read it back, but
  it is hardware that exists and is absent, rather than hardware that was ruled out.
* **The flipper End Of Stroke switches never close.** 105-108 are mapped and rest correctly, but
  only a table drives them; the bring-up scaffolding does not. The game watches them: XINA 1.10
  "fixed broken EOS logic" and 1.11 "fixed flipper control code to cut short the power coil ON
  pulse once the End Of Stroke switch closes". So with EOS never closing, the power winding stays
  driven for the full pulse where a real machine cuts it short. Nothing here measures coil-on
  time, so it costs nothing today, and it is worth knowing for anything that starts to.
* **The modulated outputs, PWM, are not implemented.** Coils, lamps and GI are reported as plain
  on/off, so a frontend gets no coil strength, no bulb fade and no brightness - `coreGlobals.nSolenoids`
  is 0 and nothing here calls `core_write_pwm_output*()` or `core_set_pwm_output_type()`. That is
  deliberate rather than forgotten, and half-doing it is worse than leaving it: `core_getSol()`
  switches to `physicOutputState` the moment `nSolenoids` is non-zero and the option is on, so
  declaring the count without also feeding the integrator would report every output as permanently
  off. See the note by `p2k_getSol` in `src/wpc/p2k.c` for what implementing it needs, and why the
  once-a-frame `p2k_sync_io()` is the thing standing in the way.
* `-frames_to_run` does not work headless - the counter it watches sits inside the display update,
  past the point where a headless run returns. The remote debugger's
  `/api/debugger/control?cmd=exit` ends a run cleanly instead, which is also what gets NVRAM
  written.
* **The driver and the firmware disagree about where the mask images are.** The four mask images -
  the games' art, `im_mask0` to `im_mask3` in the update package - live on the Prism card. This
  driver, following the MAME one, treats `0x14000000-0x14ffffff` as a single *banked* window whose
  contents are whichever of the four `m_prismdata[]` was last selected by a magic value written to
  the same window, and then maps three of them *again*, fixed, at `0x15000000`, `0x16000000` and
  `0x17000000`. So mask 0 is reachable only through the bank register and the other three are
  reachable both ways. The firmware banks nothing: `boot_im_mask_bank_is_valid` (rfm 2.22 at
  `0x2861bc`) checksums the four at fixed addresses **`0x14400000`**, `0x15000000`, `0x16000000`
  and `0x17000000`, against the sizes and checksums at `BootData` `+0x5c/+0x60`, `+0x64/+0x68`,
  `+0x6c/+0x70` and `+0x74/+0x78`. The first is the mismatch - `0x144-`, not `0x140-`, so where the
  firmware expects mask 0 this window answers from 4 MB into whatever bank is current. Nothing has
  been seen to depend on it: both games boot and play through this handler, and a normal power-up
  never validates the masks at all - the console prints the `BOOT DATA`, `SYS IMAGE`, `GAME CODE`
  and `SYMBOLS` banners and no mask one, so that routine appears to run only while an update is
  being written. Recorded because the two descriptions cannot both be right, not because it is
  known to bite. The same question has a second half: past the end of a bank the banked window
  reads 0 where the three fixed ones wrap modulo, and that is equally untested. Deciding either
  needs a machine that reads a mask through `0x14400000` with a bank other than 0 selected, or the
  Prism card's own address decode.
* **Guest writes to devices other than the interrupt controllers still take effect late.**
  `pics_settle()` runs a device's pending zero-delay timers before the next instruction, which is
  what stops an interrupt landing somewhere the guest's mask discipline says it cannot - see
  *Interrupts used to arrive late* above. It is keyed to the interrupt controllers' own ports,
  `0x20`/`0x21` and `0xa0`/`0xa1`, and two other paths reach the same machinery:

  * `update_uart_irq()` sets `pic1`'s IR4 and IR3 from the UARTs' IER write and IIR read, at
    `0x3f8-0x3ff` and `0x2f8-0x2ff`, so **IRQ3 and IRQ4 are still delivered up to a slice late**.
    Live, not theoretical: `P2K_TRAPTRACE` shows vector `0x23` - IR3 with base `0x20` - dispatched
    during boot. Nothing has been seen to suffer; the UART is a console stand-in.
  * `pit8253` defers *every* guest write. `control_w()` and `count_w()` are `synchronize()`, which
    is `timer_set(attotime::zero)`, so **programming the timer that drives the machine's clock
    only takes effect at the end of the slice**. Harmless as far as anyone has looked - the
    firmware sets divisor 298 once during boot and a slice of delay there costs nothing - but the
    exposure has not been measured. `P2K_IOWATCH=40-43` with `P2K_IOWATCH_AFTER` past boot would
    say whether it is ever reprogrammed while running.

  The fix for all of them is the same and is described below. Recorded rather than fixed because
  it costs another pass through both games.

  The fix is to stop enumerating ports and pump at the I/O boundary instead: after `port_read` in
  `port_r`, and in `port_w`, which needs splitting into a wrapper and a `port_write` the way
  `port_r` already is. Add an "already draining" guard with it - a general pump makes "no timer
  callback may perform a port access" an invisible constraint, and today's callbacks only happen
  to satisfy it (they are all internal wiring: `ir0_w`, `ir2_w`, `set_input_line`). The reason it
  is not done here is that it changes when the RTC, keyboard controller, DMA, PIT and UART timers
  land as well, so it costs another pass through both games rather than one device's worth of
  confidence.

* **`am9517a` suspends on a trigger the shim does not implement.** The DMA controller calls
  `suspend_until_trigger(1, true)` in three places and `device_t::trigger()` /
  `suspend_until_trigger()` are both empty in `shim/emu.h`, so it would not suspend and would not
  resume. Dormant as things stand - nothing wires `dreq`, `hreq` or `eop`, so no transfer ever
  starts, and the controller is only reachable through its port reads and writes. It is recorded
  because a stub that is never called looks identical to one that works, right up until someone
  wires a floppy or a sound DMA to it.
* **The update flash erases the wrong amount.** Three block sizes meet in `nvram_updates_w` and
  none agree: the CFI table the part answers with declares one region of 64 blocks of 128 KB, the
  erase command aligns on a *word* offset of `0x2000` (16 KB of bytes), and the loop clears
  `0x2000` *bytes* (8 KB). An erase therefore clears an eighth of the block the device says it has.
  It does not bite because programming is a plain store rather than the AND a real flash does, so a
  half-erased block still takes new data, and because the update image is not persisted - a bad
  erase lasts one run. Either of those changing would expose it.

* **Reading the PLX control register advances the EEPROM.** `prism_1000_r` register `0x14` - byte
  `0x50` at `0x10000050` - is the serial EEPROM's data line, and answering it moves the shift
  counter, word toggle and offset on. Anything that reads it out of band advances the transfer: a
  debugger inspecting that address, a read probe, a frontend polling memory. The firmware reads the
  whole image back and refuses to run if it does not match (*plx_ee_verify(): failed*), so one
  stray read during the transfer stops the machine booting with nothing to say why.

## Diagnostics

All of this needs a `P2K_DEBUG=1` build - see [Building](#building) - and within one is off unless
asked for. The ones that stay useful:

| | |
|---|---|
| `P2K_PROGRESS=<cycles>` | the firmware's serial console, plus cycles, instructions, host seconds, guest MIPS, interrupt counters and registers |
| `P2K_PCTRAP=<hex>[=label],…` | report when the CPU reaches an address, with registers and a cycle stamp. Two of them bracket a region and the difference is what it cost. Note the two prefixes: arming says `[p2k pctrap]`, hits say **`[p2k pc]`** |
| `P2K_QUANTUM_HZ=<hz>` | an extra cap on how long the CPU runs before device timers are looked at, **off by default** - `next_expiry()` alone bounds a slice. Not MAME's quantum; it interleaves nothing, there being one CPU in here. Set it to sweep the worst-case latency of a device change reaching the CPU, which is what the section below is about |
| `P2K_IOWATCH_AFTER=<cycles>`, `P2K_IOWATCH_MAX=<n>` | hold the I/O watch back until a cycle count, and how many accesses it then reports. A device set up once at boot and used much later needs both |
| `P2K_MEMWATCH=<from>[-<to>]` | writes to a range with the PC that made them; `P2K_MEMWATCH_CHANGED=1` for changes only |
| `P2K_READWATCH`, `P2K_DUMP`, `P2K_WATCH`, `P2K_BACKTRACE` | reads, memory dumps, per-instruction registers, a PC ring buffer |
| `P2K_DISPWATCH=1` | every change to a display controller register, plus a count of `DC_TIMING_CFG` reads. That count is how it is known nothing polls for vertical blank: **one** read per run, at boot, on both rfm_160 and swep1_210 |
| `P2K_GPWATCH=1` | every graphics pipeline register: each one's first write, then changes to the ones the blit never reads. What established that raster mode only takes 0x10c6 or 0x00cc, that no register carries a colour key, and that the pattern registers sit at `ffffffff` - the databook's precondition for the transparent copy |
| `P2K_KEYWATCH=1` | the colour key: every write carrying `0x7c1f` outside VRAM with the PC, and what the BLT buffer holds at each transparent blit. Located the key as destination data in the buffer at `0x40000400`, which is what replaced the hardcoded constant |
| `P2K_FILLWATCH=1` | solid fills only - raster 00 and ff - with the VRAM address they land on and the current framebuffer start. Quiet on every set so far: these games paint with copies, not fills |
| `P2K_SOLWATCH=1` | every coil and flasher that changes, board drivers 1-48, by name |
| `P2K_LAMPWATCH=1` | every lamp that changes, by name |
| `P2K_SWWATCH=1` | every switch that changes, by name |
| `P2K_PDBWATCH=<regs>` | power driver board traffic: reads (change-only) and writes. `1`/`all`, or a hex list like `10,11,08` - worth narrowing, register `04` is the switch row and cycles forever |
| `P2K_PCIWATCH=1` | every PCI config read, with the device number the firmware asked for |
| `P2K_BIUWATCH=1` | the four bus interface unit registers, first read and every write. What established that `BC_XMAP_1` to `_3` are never touched, so the 640K-1M decode and the absence of VGA SMI traps are safe assumptions rather than guesses |
| `P2K_IRQWATCH=1` | both interrupt masks as the guest programs them. An IRQ it unmasks and this driver never asserts is a device it will wait on for ever. Steady state on every set: master `0x60` (0, 1, 2, 3, 4, 7), slave `0x28` (8, 9, 10, 12, 14, 15) |
| `P2K_IDTDUMP=1` | the interrupt descriptor table, once, when the guest first loads a protected-mode one. Handler address, selector and gate type per hardware vector. What established that the slave PIC sits at `0x28-0x2f` here, not `0x70`, and that only IRQ 0 has a handler of its own |
| `P2K_VBLANK_IRQ=<n>` | deliver the display controller's vertical interrupt on line n, once per frame, while `VIEN` is set. Off by default and not known to be correct: the line is a guess and nothing in XINA appears to consume it. Held back until the guest has an IDT, since firing into the real-mode vector table resets the machine |
| `P2K_NAMES=1` | the machine's own device map, once at startup: PinMAME switch, coil and lamp numbers against the names the game itself uses. PinMAME has nowhere to hang these, so this is the one place they are visible. The running set picks its own table |
| `P2K_CPU_HZ=<hz>` | run the MediaGX at another clock. The default is 233/3 MHz - a compromise, the emulation being single-threaded - and raising this is how that was chosen |
| `P2K_PIT_HZ=<hz>` | move the PIT off 1.193182 MHz. How the tick handler's requirements were measured; MAME ran it at 925 kHz to suit its 20 MHz CPU |
| `P2K_CLKINT_MAX_SKIP=<n>` | how many clock interrupts `P2K_CLKINT_GATE` may hold back before one is forced through, 4 by default |
| `P2K_CLKINT_COUNTER=<hex>` | a guest address holding the firmware's own clock-handler nesting count. Non-zero there means "inside clkint", used in place of the gate's own tracking |
| `P2K_WRITEMAP=1` | a histogram of writes per megabyte of address space, dumped at exit. What showed 30 M writes landing at `0xc0800000` while the alias window stopped short of it |
| `P2K_PCTRAP_MAX=<n>` | how many `P2K_PCTRAP` reports before it goes quiet |
| `P2K_PCTRAP_AFTER=<cycles>` | hold the traps back until then. Without it a trap on anything the machine does regularly spends its budget at boot and is quiet by the time the interesting hits come round |
| `P2K_OPWATCH=1` | first use of each Cyrix CPU-access opcode (`0f 3a`-`0f 3d`) and of each register it reaches. These are MediaGX-specific, so nothing faults and nothing else logs when a set drives one |
| `P2K_DUMPAT=<hex>` | take the `P2K_DUMP` dump when the CPU reaches an address, rather than on a timer |
| `P2K_FIND=<lo>-<hi>` | instead of a hex dump, report every dword in the `P2K_DUMP` range whose value falls between lo and hi. Finding which of 55 task stacks holds a return address into a given function is otherwise 55 runs |
| `P2K_BTAT=<hex>`, `P2K_BTBELOW=<hex>` | dump the `P2K_BACKTRACE` ring at an address, or the first time the PC drops below one. A derail into low memory marches a long way before reaching any address one could name, so the useful trigger is entry into the region |
| `P2K_STACKAT=<pc>[:<n>]` | dump n dwords from the current ESP at that address, 16 by default. A fixed range cannot answer "who called this" - the stack moves |
| `P2K_STACKBELOW=<esp>` | dump the stack the first time ESP drops below a value. Runaway recursion is only readable at depth, where an address trigger would have fired on the shallow first call |
| `P2K_STACKAFTER=<cycles>` | hold the stack probes back until then. A hang looks the same on its first pass as on its millionth, but the call chain that matters is the one during the hang |
| `P2K_FORWARD=<n>` | after the CPU leaves protected mode, follow the next n instructions with registers, 40 by default |
| `P2K_HOOKTRACE=<n>` | the first n instruction-hook entries, core PC against the bridge's `REG_PC`. For checking the hook itself is armed and agreeing, not for watching code |
| `P2K_NVLOG=<hex>` | seed the firmware's error-log ring fields. They are factory-written NVRAM content, so an erased image leaves all five at zero and every diagnostic lands at linear 0, destroying the OS memory there |
| `P2K_PPM=<path>`, `P2K_PPM_AT=<cycles>` | write VRAM out as a PPM from the driver side, optionally at a cycle count. `P2K_VIDEO_PPM`/`P2K_VIDEO_PPM_AT=<frame>` are the same thing one layer up, through PinMAME's frame path - the pair tells you which layer lost the picture |
| `P2K_DSPLOG=1` | latch tracing for the sound board protocol |
| `P2K_DSPDUMP=<path>` | dump the DSP's program memory: 16K 24-bit words, one little-endian u32 each, including the scratch words its self-tests leave a diagnosis in. Taken on the first DSP reply unless a trigger narrows it |
| `P2K_DSPDUMPON=<hex>`, `P2K_DSPDUMPCMD=<hex>` | trigger that dump on a particular DSP reply, or on a particular host command, instead of the first reply |
| `P2K_DSPDUMPAFTER=<hex>` | arm the host-command trigger only after a given command has been seen, so a command that recurs can still pick out the right occurrence |
| `P2K_HITRATE=<n>`, `P2K_HITLEN=<n>` | with `P2K_PLAYFIELD`, how often a simulated playfield switch fires and how long it stays made |
| `P2K_PLAYHOLD=<n>`, `P2K_LANEHOLD=<n>` | how long `P2K_PLAY` holds a switch, and how long a lane switch stays made |
| `P2K_NO_UPDATE=1` | hide a set's update flash, so the machine boots the fallback copy in its Prism ROMs instead |
| `P2K_DCSLOG=1` | the whole conversation with the sound board |
| `P2K_VIDEO_PPM=<path>` | write the picture out as a PPM |

The following report themselves without a knob, being things that should never happen rather than
traffic worth watching. Each fires once per distinct case, on a debug build:

* `[p2k blit] raster mode XX taking the general ROP path` - 00, ff, c6 and cc have their own
  paths; anything else is evaluated from the ROP truth table. It used to draw nothing at all,
  which on screen is a hole where something belongs.
* `[p2k blit] pattern registers are not all ones` - every raster path assumes they are, which is
  what reduces c6 and cc to a source copy and is the databook's stated precondition for the
  transparent copy. The games set them once and never touch them again.
* `[p2k blit] vector mode ... asked for and not implemented` - vector mode does solid fills and
  was never implemented here or in the MAME driver this came from.
* `[p2k blit] raster XX source out of VRAM` - a copy whose source runs past the end of VRAM. The
  transparent copy reads on regardless, stepping a pixel at a time; the opaque one stops at the
  end of the buffer, because it moves the row as a block and cannot safely read past it. Either
  way the tail of that row is not what the hardware would have drawn.
* `[p2k blit] BLT buffer is not one colour` - the transparent copy takes its key from the first
  word of the BLT buffer, which is only right while the whole row holds one value. The games fill
  all 640 words with the same colour, so a row that disagrees means the hardware is being asked
  for per-column destination data, which a single key cannot express.
* `[p2k blit] transparent copy with GP_BLT_MODE RD=...` - the colour key is read from the BLT
  buffer that `GP_BLT_MODE` bits 4:2 select, and only `010` and `011` name one. `000` means no
  destination data, so all ones reach the raster unit; `100` and `101` read the destination from
  the frame buffer, which is not a colour key at all and is untested here. Either way the copy
  keys on the wrong colour instead of failing, which would look like transparency breaking in one
  place and nothing else.
* `[p2k blit] BLT without rewriting GP_DST / GP_SRC_X` - the pipeline's master/slave registers
  advance a coordinate the guest leaves alone rather than reusing it (databook Table 4-21). This
  driver reuses it, so a set that relied on that would draw one source row repeatedly.
* `[p2k gp] GP_BLT_STATUS read 200000 times with no blit` - the blitter here finishes inside the
  write that starts it, so the busy bits never read back set. A set that waits for one to *rise*
  would spin for ever, and the spin is silent otherwise: no blit, no fault, just a stopped game.

All these are silent on every set tested, including the ones whose service menu used to wedge,
which is how the blitter was ruled out as the cause of that. They are cheap to leave armed and
expensive to have to add again.

One option belongs to a different entry point: `p2k_boot.cpp` is a standalone harness that loads a
game's ROMs, runs the CPU from the reset vector and reports what the boot code touches, without
PinMAME around it. It takes its ROMs from a directory rather than from ROM regions, and
`P2K_NVRAM_UPDATES=<path>` hands it an assembled 8 MB update flash image. It is a bring-up tool,
not a test, and nothing else here uses it.

### Walking a game's own test menu

The three device watches are meant for exactly this. Enter the machine's service menu with the coin
door keys, pick a Solenoid, Lamp or Switch test, and every device the test touches prints as it
changes:

```
P2K_SOLWATCH=1 P2K_LAMPWATCH=1 P2K_SWWATCH=1 ./pinmame rfm_160

[p2k sol] frame 4213:   8 Right Popper                   on
[p2k sol] frame 4216:   8 Right Popper                   off
[p2k lamp] frame 4290:   2 Start Button                   on
[p2k sw] frame 4355:  46 Right Popper                   off
```

Names come from `src/wpc/p2k_names.h`, the running set choosing between the two games' tables, so
what prints should match what the machine puts on screen - which is what makes this a check on
those tables and not just a trace. Four things to know reading it:

* **Flashers are solenoids.** The board drives them like any other coil, so the flasher test shows
  up under `[p2k sol]`. The manual's part number for each is in `p2k_names.h` - a `#906` or `#89`
  is a bulb, the rest are coils.
* **Optos read inverted.** They rest closed and open when a ball blocks them, so `off` on switch 41
  to 47 is a ball *arriving*. The rest are the usual way round.
* **`(unnamed)`** means the device fired but no table entry covers it. On a stock machine that is
  worth looking at; on 2.x it may be one of the myPinballs additions.
* **Frame resolution.** The watches print once per frame, from `p2k_sync_io()`, and show what was
  published to PinMAME rather than the raw board state. A stroke shorter than a frame is *not*
  missed - it is accumulated on the far side of the boundary, see below - but its length is
  rounded up to the publish window, so this is not coil-accurate timing. To see the board itself,
  use `P2K_PDBWATCH`, which logs every register write as it happens.

#### Walking the switch matrix

Switches are the awkward one: the keyboard reaches the cabinet, coin door and service buttons, but
nothing on the playfield. Two ways in, both with `P2K_SWWATCH=1` and the game's own switch test on
screen:

* `P2K_PLAY="1200:sw37:10"` closes one named switch at a frame, up to sixteen per run.
* `P2K_SWSWEEP=11-88` walks the whole matrix instead, one switch at a time, with no count limit -
  which is what you want for checking names. It steps every 120 frames by default
  (`P2K_SWSWEEP=<first>-<last>[:<period>[:<hold>]]`), skips rows 0 and 9 because they do not exist,
  inverts optos for you, and starts at `P2K_SWSWEEPAT` (default 1800) so the machine is up first -
  drive a switch before that and it reads as one that broke during power-up. It repeats from the
  start, so there is time to reach the switch test by hand while it counts down.

```
P2K_SWWATCH=1 P2K_SWSWEEP=11-88 ./pinmame rfm_160
```

The bring-up log - every measurement taken to get here, and every hypothesis that turned out wrong -
is kept with the port's working notes rather than in this tree. It records how the emulation was
arrived at, not what it does.
