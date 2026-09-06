#!/usr/bin/env bash
#
# i8051_sweep.sh -- run every 8051-family game in this tree headless and record,
# per game, a checksum of everything it played plus a per-source count of the
# interrupts its sound CPU actually took.
#
# Why: check_interrupts() in src/cpu/i8051/i8051.c is shared by 36 games across
# 5 drivers, and a previous attempt to fix it shipped a regression because there
# was no way to tell whether a change altered what a game sounds like.  Run this
# before a change and after it and diff the two TSVs: a game whose audio_hash is
# unchanged is provably unaffected; a game whose hash moved has its cause named
# by whichever of the six interrupt counters moved with it.
#
# Requires a binary built with the instrumentation compiled in, and built with
# NEITHER REMOTE_DEBUG NOR DEBUG:
#
#   make -f makefile.unix I8051_SWEEP=1 -j$(nproc)      # -> ./xpinmames.x11
#
# The trailing "s" comes from I8051_SWEEP and gives the instrumented build its
# own object dir, so it can never be half-mixed with a normal xpinmame build.
#
# REMOTE_DEBUG must be OFF, and this is not a preference.  Measured on this
# tree, 2026-08-31:
#   * Speed.  REMOTE_DEBUG puts a usleep(1) inside cpu_run()'s inner loop
#     (src/cpuexec.c, "Tiny gap to allow HTTP thread to grab the lock").  One
#     game at -ftr 3600 takes 119 s with it and 5 s without -- 24x.
#   * Determinism.  It also starts a polling HTTP thread, and with it running
#     the sweep does not reproduce: over two full 35-game passes, 13 of 35 rows
#     differed, including two audio checksums.  Without it, the same games are
#     stable (e.g. bsv103's serial count was 11 in one pass and 249 in the
#     other; without REMOTE_DEBUG it is 249 three times out of three).
#   * NVRAM.  Each run is started from a cleared per-game NVRAM file (see
#     run_one below).  Without that, a game's numbers depend on how many times
#     it has been booted before, which is not something a before/after diff can
#     see or control.
# A residual +-1 jitter remains in the Alvin G games' ie1/tf0 counts (~0.05%),
# and jolypark's audio checksum still moves run to run with its counters fixed;
# see docs/findings/2026-08-31-i8051-interrupt-fix.md.
#
# Usage:
#   scripts/i8051_sweep.sh OUTPUT.tsv [BINARY] [ROMPATH]
#
# Environment overrides:
#   FTR=3600        frames to run per game (60 fps => 3600 = 60 s emulated)
#   TIMEOUT=300     wall-clock backstop per game, seconds
#   LOGDIR=<dir>    keep each game's full stdout+stderr here
#
set -u

die() { echo "i8051_sweep: $*" >&2; exit 1; }

OUT=${1:-}
[ -n "$OUT" ] || die "usage: $0 OUTPUT.tsv [BINARY] [ROMPATH]"

HERE=$(cd -- "$(dirname -- "$0")/.." && pwd)          # the pinmame checkout
BIN=${2:-$HERE/xpinmames.x11}
# The ROM sets live outside the repo, in the workspace's rompath of symlinks /
# zips.  Override with argument 2 or point it wherever they are.
ROMPATH=${3:-$(cd -- "$HERE/.." && pwd)/build/sweep-roms}

FTR=${FTR:-3600}
TIMEOUT=${TIMEOUT:-300}
LOGDIR=${LOGDIR:-}

[ -x "$BIN" ]     || die "no instrumented binary at $BIN (make -f makefile.unix I8051_SWEEP=1)"
[ -d "$ROMPATH" ] || die "no rompath at $ROMPATH"
[ -n "$LOGDIR" ] && mkdir -p "$LOGDIR"

# Sound flags.  Read this before changing them.
#
#   -fakesound is mandatory.  Without it Machine->sample_rate is 0, and
#   cpuexec.c:347 then SUSPENDS every CPU flagged CPU_AUDIO_CPU -- the 8051
#   would never execute an instruction and every number in this table would be
#   a measurement of nothing.
#
#   -nosound alone would do exactly that, which is why it must never appear on
#   its own.  Paired with -fakesound it does something different and useful: it
#   stops PinMAME opening the host audio device, so sample_rate lands on the
#   fake 8000 Hz path instead of the card's 44100, and the run is not throttled
#   to real time by the ALSA write.  The sound CPU still runs -- verified by the
#   interrupt counters being identical with and without it.
#
#   -skip_gamewarnings is mandatory too: GAME_NOT_WORKING titles (sport2k) stop
#   at a warning screen waiting for a keypress that headless can never deliver.
SNDFLAGS="-nosound -fakesound"

# Every game in this tree that instantiates an 8051-family CPU and whose ROMs are
# present, per docs/reference/i8051-test-roms.md.  Override with GAMES="a b c".
GAMES=${GAMES:-"
sport2k mephisto mephist1 uboat65
wrldtour wrldtou2 wrldtou3 mystcast mystcasa pstlpkr pstlpkr1
pmv112 pmv112r abv106 abv105 abv106r bsv103 bsv102 bsv100r bsv102r bsb105 pp100
ffv104 ffv103 ffv101 bbb109 bbb108 kpb105
bushido bushidoa bushidob mach2 mach2a jolypark vrnwrld
"}

# NVRAM is per-game persistent state and it IS written -- every game in this
# list leaves a $HOME/.xpinmame/nvram/<game>.nv behind.  It changes what the
# next run measures, and by a lot: with NVRAM carried over, bsb105 reports
# ie0=8 riti=247, and with it cleared, ie0=51 riti=265 -- a 6x difference on a
# counter this sweep exists to interpret.  bbb109's riti goes 8 -> 11 -> 23
# over successive boots from the same virgin state.  Each of those values is
# perfectly reproducible (3/3 runs) once the NVRAM state is pinned, so this is
# not emulator nondeterminism, it is a hidden input.  Clear it per game so a
# before/after comparison measures the code change and nothing else.
NVDIR=${NVDIR:-$HOME/.xpinmame/nvram}

run_one() {   # $1 = game ; leaves the captured output in $CAP
	local game=$1
	CAP=$(mktemp)
	local t0 t1
	rm -f "$NVDIR/$game.nv"
	t0=$(date +%s)
	timeout -k 5 "$TIMEOUT" "$BIN" \
		-headless $SNDFLAGS -skip_gamewarnings -skip_gameinfo \
		-ftr "$FTR" -rompath "$ROMPATH" "$game" \
		>"$CAP" 2>&1
	RC=$?
	t1=$(date +%s)
	WALL=$(( t1 - t0 ))
	[ -n "$LOGDIR" ] && cp "$CAP" "$LOGDIR/$game.log"
	return 0
}

field() {     # $1 = probe line prefix, $2 = key ; prints the value or "-"
	# Character class covers both hex digests/integer counters (e.g. "1ad5b7ff", "247")
	# and the decimal metrics added 2026-08-31 (e.g. "982.451"); a "." never appears in
	# a hex/integer field, so widening the class costs nothing for the old fields.
	sed -n "s/.*$1 .*\\b$2=\\([0-9a-f.]*\\).*/\\1/p" "$CAP" | tail -1
}

# ---------------------------------------------------------------------------
# Self-check.  A silent oracle is worse than none, so refuse to sweep unless a
# single game demonstrably produces both probe lines.
# ---------------------------------------------------------------------------
echo "i8051_sweep: self-check on mephisto (ftr=120)..." >&2
CAP=$(mktemp)
rm -f "${NVDIR:-$HOME/.xpinmame/nvram}/mephisto.nv"
timeout -k 5 120 "$BIN" -headless $SNDFLAGS -skip_gamewarnings -skip_gameinfo \
	-ftr 120 -rompath "$ROMPATH" mephisto >"$CAP" 2>&1
grep -q "I8051_SWEEP AUDIO" "$CAP" || die "binary printed no AUDIO probe line -- not an I8051_SWEEP build?"
grep -q "I8051_SWEEP IRQ"   "$CAP" || die "binary printed no IRQ probe line -- not an I8051_SWEEP build?"
grep -q "I8051_SWEEP AUDIO .* seen=0 " "$CAP" && die "AUDIO probe saw zero samples -- is -fakesound reaching the binary?"
rm -f "$CAP"
echo "i8051_sweep: self-check ok" >&2

# ---------------------------------------------------------------------------
# Status labels, corrected 2026-08-31.
#
# The original logic set status=ok as soon as loud>0 -- "at least one sample
# anywhere in sixty seconds exceeded +-1 LSB".  A constant DC offset satisfies
# that on its first sample and every sample thereafter, so a stuck AY-3-8910
# outputting a flat rail scored exactly the same "ok" as a stream that is
# actually varying.  Measured case in point: mephisto pre-fix has AC rms
# *exactly* 0.000 (476,136 of 479,066 samples pinned at +32,767) and scored
# loud=478,754, status=ok.  See .superpowers/i8051-fix-review.md section 1 for
# the full audit; that document is what this fix implements.
#
# loud==0 remains untouched and is still a valid silence test: dither can
# never exceed 1 LSB, so loud==0 really does mean nothing left the dither
# floor.  Only the converse -- treating loud>0 as meaning something -- was
# broken, and only that direction changes here.
#
# For loud>0, ac_rms (mixer.c's Welford stddev, skipping the first emulated
# second to clear the boot transient -- see the comment above
# sweep_audio_digest() in src/sound/mixer.c) now decides the label.  A rail's
# ac_rms is at most the dither floor's own ~0.5 LSB; every genuinely varying
# stream this project has measured sits at 20+ LSB.  AC_RMS_RAIL_THRESHOLD
# sits well above the first and below the second, so it does not need to be
# precise to separate the two groups correctly.  Measured on this tree:
#   - uboat65 (pure DC + dither, unchanged control):      ac_rms  0.5
#   - mephisto pre-fix (literally constant):              ac_rms  0.0
#   - pmv112 (the one game confirmed playing something,
#     a burst in seconds 2-3 then silence): ac_rms clears the threshold
#   - sport2k / mephisto post-fix (AY noise generator, not music): ac_rms in
#     the hundreds -- also NOT a rail by this test, and that is intentional.
#     "not a rail" is as far as this metric is licensed to go; it does not and
#     cannot certify that a varying signal is music rather than noise.  See
#     docs/findings/2026-08-31-i8051-interrupt-fix.md section 5 for why
#     sport2k's post-fix "not_silent" reading is a stuck noise generator, not
#     audio.
AC_RMS_RAIL_THRESHOLD=5.0

is_rail() {   # $1 = ac_rms ; true (0) if below the threshold
	awk -v v="$1" -v t="$AC_RMS_RAIL_THRESHOLD" 'BEGIN{exit !(v!="" && v+0<t)}'
}

printf 'game\trc\twall_s\taudio_hash\taudio_n\taudio_seen\tnonzero\tloud\tac_rms\tclip_pct\tac_n\tie0\ttf0\tie1\ttf1\triti\ttf2\tirq_total\tstatus\n' > "$OUT"

for game in $GAMES; do
	run_one "$game"

	hash=$(field AUDIO hash);  an=$(field AUDIO n);       aseen=$(field AUDIO seen)
	nz=$(field AUDIO nonzero); loud=$(field AUDIO loud)
	acrms=$(field AUDIO ac_rms); clippct=$(field AUDIO clip_pct); acn=$(field AUDIO ac_n)
	ie0=$(field IRQ ie0); tf0=$(field IRQ tf0); ie1=$(field IRQ ie1)
	tf1=$(field IRQ tf1); riti=$(field IRQ riti); tf2=$(field IRQ tf2)

	status=not_silent
	if [ -z "$hash" ] || [ -z "$ie0" ]; then
		# No probe line at all: the process never reached a clean teardown.
		if [ "$RC" -ge 124 ]; then status=timeout; else status=no_probe_rc$RC; fi
		hash=${hash:--}; an=${an:--}; aseen=${aseen:--}; nz=${nz:--}; loud=${loud:--}
		acrms=${acrms:--}; clippct=${clippct:--}; acn=${acn:--}
		ie0=${ie0:--}; tf0=${tf0:--}; ie1=${ie1:--}; tf1=${tf1:--}
		riti=${riti:--}; tf2=${tf2:--}; irqtot=-
	else
		irqtot=$(( ie0 + tf0 + ie1 + tf1 + riti + tf2 ))
		[ "$irqtot" -eq 0 ] && status=no_irq
		# "loud" is |sample| > 1.  Plain "nonzero" cannot be used as a silence
		# test: the mixer adds +-1 LSB of TPDF dither to every sample whether or
		# not anything is playing, so a silent machine still shows nonzero for
		# about half its samples.  Dither can never exceed 1 LSB, so loud==0 is
		# a real silence test.
		if [ "$loud" -eq 0 ]; then
			if [ "$status" = no_irq ]; then status=silent_no_irq; else status=silent; fi
		elif is_rail "$acrms"; then
			# loud>0 but the signal doesn't vary: a DC offset (at any level)
			# plus dither, not something worth calling "not silent".
			if [ "$status" = no_irq ]; then status=rail_no_irq; else status=rail; fi
		fi
		# else: loud>0 and ac_rms clears the rail threshold -- leave status at
		# "not_silent".  That is a claim about variance, not about music; see
		# the comment block above.
	fi

	printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
		"$game" "$RC" "$WALL" "$hash" "$an" "$aseen" "$nz" "$loud" \
		"$acrms" "$clippct" "$acn" \
		"$ie0" "$tf0" "$ie1" "$tf1" "$riti" "$tf2" "$irqtot" "$status" >> "$OUT"
	printf '%-10s rc=%-3s %3ss  hash=%-8s loud=%-8s ac_rms=%-8s clip%%=%-7s irq=%-8s %s\n' \
		"$game" "$RC" "$WALL" "$hash" "$loud" "$acrms" "$clippct" "$irqtot" "$status" >&2

	rm -f "$CAP"
done

echo "i8051_sweep: wrote $OUT" >&2
