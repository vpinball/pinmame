#!/usr/bin/env bash
#
# cirsa_sound_sweep.sh -- walk every switch element of sport2k / mephisto /
# mephist1 and record which of them make the ROM queue a sound command.
#
# This is the harness behind docs/findings/2026-09-04-switch-sounds.md.  The
# stimulus itself lives in the driver (src/wpc/mephisto.c, CIRSA_SNDSWEEP) so
# that it is scheduled in EMULATED frames and nothing about the answer can
# depend on how fast the host happens to be.  Build it with:
#
#   make -f makefile.unix I8051_SWEEP=1 CIRSA_SNDSWEEP=1 -j$(nproc)
#                                                     # -> ./xpinmamesc.x11
#
# NEITHER REMOTE_DEBUG NOR DEBUG -- see scripts/i8051_sweep.sh for the numbers.
# The trailing "sc" gives this configuration its own object dir, so it can never
# be half-mixed with a plain build: make watches timestamps, not CFLAGS.
# The instrumented binary still behaves normally unless CIRSA_SWEEP=1 is in the
# environment, which this script sets.
#
# Usage:
#   scripts/cirsa_sound_sweep.sh OUTDIR [ROMPATH]
#   CLOSURES="120 300 500 1200" SLOW=2500 GAMES="mephisto" scripts/... OUTDIR
#
# One capture per (game, closure).  Closures are in milliseconds and are the
# variable under test: a ball resting in a scoop holds a switch for seconds, a
# coin chute must be released inside the ROM's stuck-coin window (~0.58 s on
# sport2k, ~0.27 s on either Mephisto -- docs/findings/2026-09-04-mephisto-coin-
# window.md), so no single closure length surveys the machine.  The SLOW capture
# also changes the SLOT length, which changes the pass period, and that is the
# discriminator that separates a switch sound from a ROM timer that happens to
# beat against the pass period.
#
# The keep-alive coin closure is fixed at 120 ms in every capture: it is harness
# plumbing, not the variable, and at 300 ms Mephisto refuses it about half the
# time.
set -u

HERE=$(cd -- "$(dirname -- "$0")/.." && pwd)
OUT=${1:-}
[ -n "$OUT" ] || { echo "usage: $0 OUTDIR [ROMPATH]" >&2; exit 1; }
ROMPATH=${2:-$(cd -- "$HERE/.." && pwd)/build/roms}
BIN=${BIN:-$HERE/xpinmamesc.x11}
GAMES=${GAMES:-"sport2k mephisto mephist1"}
CLOSURES=${CLOSURES:-"120 300 500 1200"}
SLOW=${SLOW:-2500}
SLOW_SLOT=${SLOW_SLOT:-4000}
SLOT=${SLOT:-1700}
PASSES=${PASSES:-5}
BOOT=${BOOT:-40000}
SLOTS=77                    # 68 elements + one keep-alive every 8 of them

[ -x "$BIN" ]     || { echo "no instrumented binary at $BIN" >&2; exit 1; }
[ -d "$ROMPATH" ] || { echo "no rompath at $ROMPATH" >&2; exit 1; }
mkdir -p "$OUT"

capture() {   # $1 game  $2 closure_ms  $3 slot_ms
  local game=$1 close=$2 slot=$3
  local ftr=$(( BOOT*60/1000 + PASSES*SLOTS*(slot*60/1000) + 180 ))
  # A private HOME per capture: NVRAM is a hidden input (scripts/i8051_sweep.sh
  # documents how much it moves the numbers) and captures run in parallel.
  local run; run=$(mktemp -d /tmp/cirsasweep.XXXXXX)
  mkdir -p "$run/.xpinmame/nvram"
  ( cd "$HERE" || exit 1
    HOME=$run CIRSA_SWEEP=1 SWEEP_BOOT_MS=$BOOT SWEEP_SLOT_MS=$slot \
    SWEEP_CLOSE_MS=$close SWEEP_COIN_MS=120 SWEEP_PASSES=$PASSES \
    timeout -k 5 3600 "$BIN" -headless -nosound -fakesound -skip_gamewarnings \
        -skip_gameinfo -ftr "$ftr" -rompath "$ROMPATH" "$game" \
        > "$OUT/$game.c$close.log" 2>&1 )
  echo "  $game close=${close}ms slot=${slot}ms -> $OUT/$game.c$close.log" >&2
  rm -rf "$run"
}

for g in $GAMES; do
  for c in $CLOSURES; do capture "$g" "$c" "$SLOT" & done
  capture "$g" "$SLOW" "$SLOW_SLOT" &
done
wait
echo "captures in $OUT; now: scripts/cirsa_sound_table.py $OUT" >&2
