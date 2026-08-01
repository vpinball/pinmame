# PinMAME Remote Debugger

A thread-safe remote debugging extension for PinMAME with a web-based
dashboard and REST API. Designed to be minimally invasive: all changes to
existing PinMAME sources are guarded by `#ifdef REMOTE_DEBUG`; without that
define the build is identical to upstream.

Developed and tested against **Williams WPC**, but most features are
platform-generic and work across PinMAME's other pinball systems to varying
degrees — see [Platform Support](#platform-support).

## Features
- **Headless Operation**: run without X11 via `-headless` (server/CI/reverse
  engineering setups); the debugger is the only frontend.
- **Thread Safety**: the emulator thread holds a recursive lock for each CPU
  timeslice; the HTTP thread takes the same lock around all machine access.
- **REST API** with proper HTTP status codes (400/404/500/503 on errors) and
  a self-describing reference at `/api/doc`.
- **Push events (SSE)**: `GET /api/events` streams halt/resume/step/message
  events; the web UI uses it and falls back to polling automatically.
- **High-Fidelity Rendering**:
  - Full DMD support (128x32 raw frame capture).
  - Hardware-accurate Williams 16-segment alphanumeric starburst rendering.
- **Matrix Visualizer**: live lamps, switches, solenoids, dedicated inputs.
- **Interactive Inputs**: pulse cabinet buttons and toggle matrix switches.
- **Advanced Debugging**:
  - Bank-aware disassembler incl. **backwards disassembly** (`before=N`).
  - Callstack tracking with full register context and ROM bank.
  - Banked, **conditional** breakpoints (`cond=A==FF`) with hit/ignore counts.
  - Watchpoints covering a **byte range** (R/W/RW), optionally with a
    **value condition** (halt only when the byte reaches a value).
  - Step Into / **Step Over** / **Step Out** / Run-To.
  - Memory hex editor, pattern search, block fill, **block write**
    (`data=DEADBEEF`). Debugger writes bypass the WPC RAM write protection.
  - **Memory access trace**: log every access to selected addresses.
  - **Code instrumentation**: count how often execution passes marked
    addresses (bank-aware PC hit counting).
  - **Execution trace**: ring buffer of the last N executed instructions
    (PC/bank/regs) — "how did I get here" after a halt.
  - **Code coverage**: record which addresses executed (per bank); the web
    UI can highlight covered lines in the disassembler.
  - **Tracepoints**: log a register snapshot every time PC hits an address
    and keep running (non-intrusive tracing).
  - **Value scan**: cheat-engine style variable search (snapshot + filter
    changed/unchanged/inc/dec/equals).
  - **Save states**: lightweight in-memory checkpoints of WPC RAM + main
    CPU registers (save/load reliably, also while paused).
  - **DMD recorder**: capture a sequence of DMD frames and replay them.
  - NVRAM management (dump/clear).
- **Playfield objects**:
  - Named switch / lamp / solenoid enumeration (`/api/switches`,
    `/api/lamps`, `/api/solenoids`) using the WPC switch numbering
    (`col*10 + row + 1`) plus standard dedicated names.
  - Timed switch **pulses** (hold for N ms, wall clock).
  - **Object monitoring**: watch selected lamps/solenoids/switches and
    retrieve a timestamped action log of their state changes, optionally
    **halting** the emulator when a watched object changes.

## Setup & Build
1. `REMOTE_DEBUG = 1` is set near the top of `makefile.unix` (fork default).
   `DEBUG = 1` stays enabled as well: the disassembler endpoint needs
   `MAME_DEBUG` for real 6809 disassembly (without it only byte dumps are
   returned) and it names the binary `xpinmamed.*`.
2. Build: `make -f makefile.unix`.
3. Run: `./xpinmamed.x11 -headless -startpaused -httpport 8926 -nosound -rompath <dir> <romname>`
4. Open the dashboard: `http://localhost:8926/ui`.

Command line options added by the debugger (see `src/unix/config.c`):
`-headless`/`-hl`, `-httpport <port>`/`-hp` (default 8080), `-startpaused`/`-sp`.

### Code quality
The `src/remote_debug` objects are compiled with
`-std=gnu99 -Wall -Wextra -Wunused -pedantic -Werror` (see `src/pinmame.mak`);
the build fails on any warning in this directory. Core PinMAME headers are
included as system headers for these objects so legacy-code warnings do not
leak in.

## Usage Examples

All examples assume the emulator is running headless on port 8926:

```sh
./xpinmamed.x11 -headless -startpaused -httpport 8926 -nosound \
    -rompath ~/.pinmame taf_l7 &
BASE=http://localhost:8926
```

**Inspect and step the CPU while halted**
```sh
curl -s "$BASE/api/debugger/control?cmd=pause"
curl -s "$BASE/api/debugger/state"                     # registers of all CPUs
curl -s "$BASE/api/debugger/dasm?addr=8000&lines=8&before=2"
curl -s "$BASE/api/debugger/control?cmd=step"          # step, then re-read state
curl -s "$BASE/api/debugger/state/write?reg=A&val=7F"  # poke accumulator A
```

**Break on an address (bank-aware) and watch for the hit via SSE**
```sh
curl -s "$BASE/api/debugger/breakpoints?cmd=add&addr=8CC1"      # any bank
curl -s "$BASE/api/debugger/breakpoints?cmd=add&addr=6000&bank=3E"  # bank 3E only
curl -s "$BASE/api/debugger/control?cmd=resume"
curl -sN "$BASE/api/events"      # streams {"event":"halt","reason":"bp",...}
```

**Conditional breakpoint (halt only when X > 0x1000, after 5 hits)**
```sh
curl -s "$BASE/api/debugger/breakpoints?cmd=add&addr=9000&cond=X>1000&ignore=5"
```

**Find a variable in RAM (value-scan / cheat-engine workflow)**
```sh
curl -s "$BASE/api/debugger/scan?cmd=new&addr=0000&size=8192"   # snapshot RAM
# ... do something in game that changes the value ...
curl -s "$BASE/api/debugger/scan?cmd=filter&op=changed"         # keep changed bytes
# ... change it again ...
curl -s "$BASE/api/debugger/scan?cmd=filter&op=changed"         # narrow further
curl -s "$BASE/api/debugger/scan"                               # remaining candidates
```

**Count how often a routine runs (code instrumentation)**
```sh
curl -s "$BASE/api/debugger/instrument?cmd=add&addr=A2F0"
curl -s "$BASE/api/debugger/control?cmd=resume"; sleep 5
curl -s "$BASE/api/debugger/instrument"        # [{"addr":41712,"bank":-1,"count":137}]
```

**Monitor objects and read the action log**
```sh
curl -s "$BASE/api/monitor?cmd=add&type=lamp&id=11"    # watch lamp 11
curl -s "$BASE/api/monitor?cmd=add&type=sol&id=5"      # watch solenoid 5
curl -s "$BASE/api/monitor/log"    # [{"type":"lamp","id":11,"val":1,"t":48213}, ...]
```

**Inject switches (pulse a coin, toggle a target)**
```sh
curl -s "$BASE/api/switches"                       # list with numbers + names
curl -s "$BASE/api/input?sw=1&val=1&pulse=150"     # pulse Coin 1 for 150 ms
curl -s "$BASE/api/input?sw=57&val=1"              # hold switch 57 on
```

**Checkpoint the game state, poke around, restore it**
```sh
curl -s "$BASE/api/debugger/savestate?cmd=save&slot=before"
curl -s "$BASE/api/debugger/memory/write?addr=0500&data=00000000"
curl -s "$BASE/api/debugger/savestate?cmd=load&slot=before"   # RAM + CPU restored
```

**Record and download DMD frames**
```sh
curl -s "$BASE/api/debugger/dmdrec?cmd=start"
curl -s "$BASE/api/debugger/control?cmd=resume"; sleep 3
curl -s "$BASE/api/debugger/dmdrec?cmd=stop"          # {"count":180,...}
curl -s "$BASE/api/debugger/dmdrec/data" -o dmd.bin   # packed binary frames
```

**Dump / clear NVRAM (WPC)**
```sh
curl -s "$BASE/api/debugger/nvram/dump" -o nvram.bin
curl -s "$BASE/api/debugger/nvram?cmd=clear"
```

See `test_suite.sh` for an end-to-end script exercising every endpoint, and
`find_nvram_coins.sh` for a complete reverse-engineering example (locating
the coin/credit counters in CMOS with the value scan — insert coins and
alternate `inc`/`unchanged` filters to drop the RTC noise).

## API Reference

Everything is `GET`. Parameter conventions:

| Parameters | Base |
|------------|------|
| `addr`, `val`, `bank`, `pattern`, `data`, `cond` values | hexadecimal, `0x`/`$` prefix optional |
| `size`, `lines`, `before`, `cpu`, `sw`, `idx`, `len`, `mode`, `ignore` | decimal |

The live reference is served at `GET /api/doc`.

### ROM banking and the `bank` parameter

WPC memory is laid out as:

| CPU address range | Contents | Banked? |
|---|---|---|
| `0x0000`-`0x3FFF` | RAM / ASIC I/O | no |
| `0x4000`-`0x7FFF` | paged ROM (16 KB window) | **yes** |
| `0x8000`-`0xFFFF` | fixed "prime" ROM | no |

Only the `0x4000`-`0x7FFF` window is bank-switched, so the `bank` parameter
is only meaningful for addresses in that range. For RAM/ASIC (`0x0000`-
`0x3FFF`) and prime ROM (`0x8000`-`0xFFFF`) you do **not** pass a bank; if
you do, it is ignored (those addresses are the same in every bank).

`bank` is always **hexadecimal** (e.g. `bank=1E`, `bank=0x20`). Omitting it
(or passing `-1`) means "current bank" for reads, and "any bank" for
break/watch/trace points (they match regardless of which bank is paged in
when the address is hit).

Address-based endpoints and their `bank` support:

| Endpoint | `bank`? | Meaning |
|---|---|---|
| `breakpoints` (add) | yes | halt at addr only when this bank is paged in |
| `watchpoints` (add) | yes | trigger only when this bank is paged in |
| `control/runto` | yes | temporary breakpoint honoring the bank |
| `instrument` (add) | yes | count only hits in this bank |
| `trace` (add) | yes | log accesses only in this bank |
| `dasm` | yes | disassemble that bank mapped into the window |
| `memory` (read) | yes | read the window from that ROM bank (side-effect free) |
| `memory/find` | yes | search within that ROM bank |
| `memory/write`, `memory/fill` | no | writes target RAM/ASIC, which is unbanked |
| `scan` | no | value search operates on RAM, which is unbanked |

Example — set a breakpoint at `0x5120` only while ROM bank `0x20` is active
(vs. an all-banks breakpoint):

```sh
curl -s "$BASE/api/debugger/breakpoints?cmd=add&addr=5120&bank=20"   # bank 0x20 only
curl -s "$BASE/api/debugger/breakpoints?cmd=add&addr=5120"           # any bank
```

On non-WPC platforms there is no banking; the `bank` parameter is accepted
but ignored (reads always go through the current memory map).

### System Info & Captures
- `GET /api/info`: JSON with game status, lamps, switches, segments, solenoids.
- `GET /api/events`: Server-Sent-Events stream. Each event is a JSON object:
  `{"event": "halt"|"resume"|"step"|"message"|"quit", ...}` with context
  fields (`reason`, `pc`, `bank`, `text`) where applicable.
- `GET /ui`: serves the web dashboard (HTML).

#### DMD
- `GET /api/dmd/info`: `{width, height}`.
- `GET /api/dmd/raw`: raw frame, 1 luminance byte per pixel.
- `GET /api/dmd/pnm`: PGM (P5) image.

#### Screenshot
- `GET /api/screenshot/info`: `{width, height}`.
- `GET /api/screenshot/raw`: raw RGB24.
- `GET /api/screenshot/pnm` (alias `/api/screenshot`): PPM (P6) image.

### Debugger Control
- `GET /api/debugger/control?cmd=[pause|resume|step|stepover|stepout|exit]`
- `GET /api/debugger/control/runto?addr=HEX[&bank=HEX]`
- `GET /api/debugger/state`: registers/flags of all CPUs.
- `GET /api/debugger/state/write?reg=[NAME|ID]&val=HEX[&cpu=N]`:
  set a register. M6809 names (PC,S/SP,CC,A,B,U,X,Y,DP) and ADSP2100 names
  (PC,AX0..,MR0..,CNTR,ASTAT,...) are resolved per CPU type.
- `GET /api/debugger/dasm?addr=HEX[&lines=N][&before=N][&cpu=N][&bank=HEX]`:
  JSON disassembly. `before=N` prepends up to N instructions *before* `addr`
  (heuristic backtracking). `bank` temporarily maps that WPC ROM bank.
- `GET /api/debugger/messages`: message log.
- `GET /api/debugger/callstack`: list of `{caller, receiver, bank, pc(=return
  address), u, s, x, y, a, b, dp, cc}`.

### Points (Breakpoints & Watchpoints)
- `GET /api/debugger/points`: JSON list of all BPs and WPs, including
  `cond`, `hits`, `ignore`, `temp` (BP) and `len` (WP).
- `GET /api/debugger/points?cmd=[toggle|delete]&type=[bp|wp]&idx=N`
- `GET /api/debugger/breakpoints?cmd=add&addr=HEX[&bank=HEX][&cond=EXPR][&ignore=N]`:
  `EXPR` compares an M6809 register with a hex constant using
  `==`, `!=`, `<`, `>`, `<=`, `>=` (e.g. `cond=A==7F`, `cond=X>1000`).
  `ignore=N` skips the first N (condition-true) hits.
- `GET /api/debugger/breakpoints?cmd=clear`
- `GET /api/debugger/watchpoints?cmd=add&addr=HEX[&len=N][&mode=1|2|3][&bank=HEX][&cond=eq|ne|lt|gt|le|ge&val=HEX]`:
  watch `len` bytes (default 1); mode 1=read, 2=write, 3=both;
  `bank` restricts to that ROM bank (0x4000-0x7FFF only). With `cond`+`val`
  the watchpoint only halts when the byte at `addr` satisfies the comparison
  (e.g. `mode=2&cond=eq&val=05` = "break on the write that sets it to 5").
  Note: debugger writes go straight to RAM and do not trigger watchpoints —
  only the emulated CPU's own accesses do.
- `GET /api/debugger/watchpoints?cmd=clear`

### Memory, Trace & NVRAM
- `GET /api/debugger/memory?addr=HEX[&size=N][&cpu=N][&bank=HEX]`: read
  (max 2048 bytes); `bank` reads the 0x4000-0x7FFF window from that ROM bank.
- `GET /api/debugger/memory/write?addr=HEX&val=HEX[&cpu=N]`: write one byte.
- `GET /api/debugger/memory/write?addr=HEX&data=HEXBYTES[&cpu=N]`: block write
  (e.g. `data=DEADBEEF`, max 256 bytes).
- `GET /api/debugger/memory/fill?addr=HEX&size=N&val=HEX[&cpu=N]`
- `memory/write` and `memory/fill` take **no** `bank` — their targets are
  RAM/ASIC (`0x0000`-`0x3FFF`), which is unbanked, and the banked window is
  read-only ROM.
- `GET /api/debugger/memory/find?pattern=HEXBYTES[&addr=HEX][&size=N][&cpu=N][&bank=HEX]`
  (`bank` searches within that ROM bank)
- Note: debugger writes into the WPC RAM range go directly to RAM and are
  **not** blocked by the WPC memory protection hardware simulation.
- `GET /api/debugger/trace?cmd=add&addr=HEX[&bank=HEX]`: trace accesses to an
  address (`bank` restricts to that ROM bank).
- `GET /api/debugger/trace?cmd=clear` / `GET /api/debugger/trace`: clear /
  list `{watched: [{addr, bank}, ...], logs: [{cpu, pc, adr, len, write, bank}, ...]}`.
- `GET /api/debugger/nvram/dump`: raw 8KB WPC CMOS dump.
- `GET /api/debugger/nvram?cmd=clear`: wipe NVRAM (machine reset required to
  reinitialize).

### Playfield Objects
Switches, lamps and solenoids use the WPC number `col*10 + row + 1`
(matrix 11-88; coin-door column 1-8; flipper column 111-118).
- `GET /api/switches`: all switches as `{num, col, row, active, name}`.
  Names are the WPC standard dedicated/cabinet names where known.
- `GET /api/lamps`: all lamps as `{num, col, row, active}`.
- `GET /api/solenoids`: all solenoids as `{num, active}`.

Note: the coin-door column (switches 1-8) is refreshed from the emulated
input port every frame, so a plain level set only persists while the game
runs when re-asserted — use a pulse for coins.

### Object Monitoring
- `GET /api/monitor?cmd=add&type=[sw|lamp|sol]&id=N[&break=1]`: watch an
  object; with `break=1` the emulator is paused on the next frame after the
  object changes (frame-granular — to pin the exact writing instruction, use
  a write watchpoint on the underlying I/O register instead).
- `GET /api/monitor?cmd=clear` / `GET /api/monitor`: clear / list monitors.
- `GET /api/monitor/log`: timestamped action log
  `{actions: [{type, id, val, t(ms)}, ...]}`; `?cmd=clear` resets it.
  State changes are also pushed as `{"event": "action", ...}` SSE events;
  a break also pushes `{"event": "halt", "reason": "monitor", ...}`.

### Execution Trace
Ring buffer of the most recently executed instructions — fetch it after a
halt to see how execution got there.
- `GET /api/debugger/exectrace?cmd=[start|stop|clear]`
- `GET /api/debugger/exectrace`: `{enabled, count, capacity, trace: [{pc,
  bank, a, b, x}, ...]}` (oldest to newest; capacity 8192).

### Code Coverage
Record which addresses executed. Banked-window code (0x4000-0x7FFF) is
tracked per bank so different banks are not conflated.
- `GET /api/debugger/coverage?cmd=[start|stop|clear]`
- `GET /api/debugger/coverage`: `{enabled, executed, addressable}`.
- `GET /api/debugger/coverage?addr=HEX[&size=N][&bank=HEX]`: per-address
  executed flags `{addr, bank, executed: [0|1, ...]}` (like a memory read).
  The web UI uses this to highlight executed lines in the disassembler.

### Tracepoints
Like breakpoints, but log a register snapshot and keep running.
- `GET /api/debugger/tracepoints?cmd=add&addr=HEX[&bank=HEX]`
- `GET /api/debugger/tracepoints?cmd=clear`
- `GET /api/debugger/tracepoints`: `{points: [{addr, bank, hits}, ...],
  log: [{pc, bank, a, b, x, y, u, s, dp, cc}, ...]}` (log capacity 512).

### Code Instrumentation (PC hit counting)
- `GET /api/debugger/instrument?cmd=add&addr=HEX[&bank=HEX]`: count passes
  over a code address (bank -1/omitted matches any bank).
- `GET /api/debugger/instrument?cmd=clear` / `GET /api/debugger/instrument`:
  clear / list `{points: [{addr, bank, count}, ...]}`.

### Value Scan (variable search)
Classic cheat-engine workflow to locate a variable in RAM (no `bank`
parameter — variables live in the unbanked RAM/ASIC region `0x0000`-`0x3FFF`):
- `GET /api/debugger/scan?cmd=new&addr=HEX[&size=N][&cpu=N]`: snapshot a
  region (size 1..8192); every byte becomes a candidate.
- `GET /api/debugger/scan?cmd=filter&op=[eq|ne|changed|unchanged|inc|dec][&val=HEX]`:
  keep candidates matching the comparison against the previous snapshot
  (`changed`/`inc`/...) or a value (`eq`/`ne`). Repeat to narrow down.
- `GET /api/debugger/scan`: current `{count, results: [{addr, val}, ...]}`
  (results capped at 256).

### Save States
Lightweight checkpoints of the game logic state (WPC RAM + main CPU
registers), kept in memory in up to 8 named slots. This intentionally does
**not** use MAME's full state-save machinery, which is incomplete for
WPC/DCS and crashes on this driver; the checkpoint captures exactly what is
useful for reverse engineering and works reliably, including while paused.
It does not restore sound/DMD hardware state.
- `GET /api/debugger/savestate?cmd=save&slot=NAME`
- `GET /api/debugger/savestate?cmd=load&slot=NAME`
- `GET /api/debugger/savestate?cmd=delete&slot=NAME`
- `GET /api/debugger/savestate`: list slots `{slots: [{name, pc}, ...]}`.
- `GET /api/debugger/savestate/diff?a=SLOT[&b=SLOT]`: diff slot `a`'s RAM
  against slot `b`, or against the live RAM when `b` is omitted —
  `{a, b, count, diffs: [{addr, a, b}, ...]}` (up to 1024 entries). Handy
  for "what changed between these two moments".

### DMD Recorder
Capture DMD frames into a ring buffer (one per emulated video frame,
capacity ~512 frames) and download them for playback/analysis.
- `GET /api/debugger/dmdrec?cmd=[start|stop|clear]`
- `GET /api/debugger/dmdrec`: status `{recording, count, width, height, capacity}`.
- `GET /api/debugger/dmdrec/data`: packed binary of all frames —
  little-endian `u32 count, u32 width, u32 height`, then per frame
  `u32 t_ms` followed by `width*height` luminance bytes.

### Input & Classic Commands
- `GET /api/input?sw=N&val=[0|1][&pulse=MS]`: set a cabinet/matrix switch;
  with `pulse=MS` it holds the value for MS milliseconds then restores the
  opposite (works for coin switches too, re-asserted each frame).
- `GET /api/debugger/command?cmd=STRING`: classic MAME-style commands
  (URL-encoded): `BP [bank:]addr`, `BC`, `WP [bank:]addr[,len[,type]]`, `WC`,
  `G`, `S`, `F addr,len,val`, `QUIT`, `HELP` — all values hex.

## Web UI
`http://localhost:<port>/ui` — single-file dashboard (vanilla JS):
- SSE push with automatic fallback to adaptive polling (fast while running,
  slow while paused/offline), offline banner and error toasts.
- Keyboard shortcuts: `F8` pause/resume, `F10` step over, `F11` step into,
  `Shift+F11` step out.
- Disassembler with auto-follow-PC (incl. context lines above PC), click a
  line to prefill the breakpoint form.
- Registers editable by clicking the hex value.
- Hex editor: inline byte editing (Enter commits, Esc cancels), page up/down,
  changed-byte highlighting, auto-refresh on halt, pattern search with
  "Next", NVRAM view preset and NVRAM dump download.
- Watches panel (persisted in the browser) and memory trace panel.
- Conditional breakpoints with hit counters in the points list.
- Cabinet/service buttons built from the named switches, with a
  configurable pulse duration.
- Switch matrix tooltips show the switch name; Shift+click a switch to
  assign a custom label (stored in the browser).
- Code instrumentation, value scan and object monitor / action log panels.
- Execution-trace, code-coverage and tracepoint panels; coverage can
  highlight executed lines directly in the disassembler.
- Watchpoint value-condition inputs; monitor "break on change" checkbox;
  save-state "diff vs live" button.
- Save-state slots and a DMD recorder with in-browser frame playback.
- Authoritative 16-segment alphanumeric rendering: the glyph pixel maps are
  decoded from PinMAME's own core segment table, so the display matches the
  emulator exactly (all 16 segments incl. commas/periods).

## Platform Support

The debugger was developed and verified against **Williams WPC** (`taf_l7`).
Most of it is *platform-generic*, because it hooks PinMAME's shared
subsystems rather than WPC-specific code:

- the per-instruction debug hook (`CALL_MAME_DEBUG`) → breakpoints, stepping,
  code instrumentation;
- the generic memory access macros (`memory.c`) → watchpoints, value scan,
  memory trace;
- the core object state (`coreGlobals` switches/lamps/solenoids/segments and
  `core_get_dmd_data`) → matrices, segment/DMD rendering, monitoring.

A few things are WPC-specific and *degrade gracefully* on other platforms
(they check for the WPC RAM/banking and return `-1`, an empty value, or a
`404`/`500` instead of crashing): bank-aware breakpoints & disassembly, the
binary NVRAM dump, save states, the "dedicated" switch field in `/api/info`,
and the WPC memory-protection bypass on writes. The callstack is built from
hooks that live only in the **M6809** CPU core.

### Support tiers

**Tier 1 — Full (developed & tested): Williams WPC** (Alpha → DMD → WPC-95),
M6809 main CPU. Every feature works, including bank-aware breakpoints/
disassembly, callstack, NVRAM dump and save states.

**Tier 2 — M6809-based (untested, expected full run-control + callstack):**
Sega/Stern Whitestar (`se.c`), Wico, Barni, Ice Cold Beer, Bally video
(video CPU); also the M6809 DMD sound board of Data East / Whitestar. All
debugging works; the WPC-only endpoints above are unavailable (save states
and NVRAM dump rely on the WPC RAM pointer).

**Tier 3 — Other CPUs *with* the instruction hook (run control +
breakpoints + all generic tools; no callstack; register *names* only for
M6809/ADSP2105, others by numeric id):** Williams System 3-11, all Data East
main CPUs, Bally MPU-17/35/6803, Gottlieb System 1/80/3, Atari, Zaccaria,
Capcom, Alvin G., Spinball, Inder, Mr. Game and most of the smaller
Z80/M6800/M6502/M68000 systems. CPU families covered by the hook:
M6800/02/03/08, M6502/M65C02, Z80, S2650, SCAMP, M68000/M68306, PPS4, i8085,
i86, i8039, CDP1802, i4004, COP420, TMS7000, i8051.

**Tier 4 — Limited, *no* instruction hook:** Stern **S.A.M.** (AT91/ARM7),
**NSM** and the **JVH** main CPU (TMS99xx). These CPU cores do not emit
`CALL_MAME_DEBUG`, so PC breakpoints, step-over/run-to and code
instrumentation do not fire. Still available: pause/resume, memory
read/write/fill/find, watchpoints and value scan (via the generic memory
hooks), switches/lamps/solenoids, DMD/segments, monitoring, screenshots and
numeric register access.

### Feature availability

| Feature | Works on |
|---|---|
| Pause / resume / single step, `/api/info`, matrices, screenshot | all platforms |
| Switches / lamps / solenoids, input, pulse, monitoring, value scan, memory read/write/fill/find, watchpoints, memory trace | all platforms |
| DMD info/raw/pnm + recorder | any DMD game |
| Segment display (`/api/info` segments, UI rendering) | any alphanumeric game |
| Disassembly (`/api/debugger/dasm`, unbanked) | any CPU with a MAME disassembler (needs `DEBUG=1`, the fork default) |
| PC breakpoints, conditional BP, step-over, run-to, code instrumentation, execution trace, coverage, tracepoints | CPUs that emit the instruction hook (Tier 1-3; **not** Tier 4) |
| Watchpoint value condition, monitor break-on-change | all platforms (watchpoints/monitoring are generic) |
| Save-state diff | WPC only (uses the WPC RAM checkpoint) |
| Register *names* in `state/write` | M6809 and ADSP2105 (others: numeric id) |
| Callstack, step-out | M6809 core only (Tier 1-2) |
| Bank-aware breakpoints / disassembly, NVRAM binary dump, save states, dedicated-switch field | WPC only |
| NVRAM clear | any driver with an `nvram_handler` |

Non-WPC platforms are untested — the analysis above is from the code paths,
not from running each ROM. Switch/lamp/solenoid *names* are WPC-specific;
on other platforms the objects are still enumerated by matrix number, just
without a friendly name (assign your own via the UI or ignore the `name`).

## Testing
Run from `src/remote_debug` (ROM `taf_l7` expected in `~/.pinmame`, override
via `ROMPATH=/path`):
- `./test_suite.sh` — full API verification suite.
- `./test_breakpoint.sh` — end-to-end breakpoint hit test.
- `./re_demo.sh` — guided tour of the RE tooling (coverage, execution
  trace, tracepoints, value-condition watchpoint with callstack, monitor
  break-on-change, save-state diff) on a running game.
- `./find_nvram_coins.sh` — example RE workflow: locate the coin/credit
  counters in NVRAM using the value scan (insert coins, alternate
  `inc`/`unchanged` filters). Pretty-prints with python3 if present.

## Misc

### CPU Register Mapping (M6809)
| Index | Name | Description |
|-------|------|-------------|
| 1     | PC   | Program Counter |
| 2     | S    | Stack Pointer (SP) |
| 3     | CC   | Condition Codes (FLAGS) |
| 4     | A    | Accumulator A |
| 5     | B    | Accumulator B |
| 6     | U    | User Stack Pointer |
| 7     | X    | Index Register X |
| 8     | Y    | Index Register Y |
| 9     | DP   | Direct Page Register |

### Williams 16-Segment Mapping
The alphanumeric renderer uses the hardware mapping (bits 0-indexed):
- **Bits 0-5**: outer segments (a, b, c, d, e, f)
- **Bit 6**: middle-left horizontal (g1), **Bit 11**: middle-right (g2)
- **Bit 9**: center-top vertical (h), **Bit 13**: center-bottom (i)
- **Bit 8**: diag top-left (j), **Bit 10**: diag top-right (k)
- **Bit 14**: diag bottom-left (l), **Bit 12**: diag bottom-right (m)
- **Bit 15**: period, **Bit 7**: comma

### Known warnings
None. The remote_debug objects build warning-free with
`-std=gnu99 -Wall -Wextra -Wunused -pedantic -Werror`.
