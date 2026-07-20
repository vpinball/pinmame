#!/bin/bash
# WPC NVRAM coin-counter finder — a worked reverse-engineering example.
#
# Locates the coin/credit counters in the battery-backed RAM by using the
# debugger's value-scan (cheat-engine style) rather than a raw dump diff.
# A plain "insert coins, diff the NVRAM" comparison is swamped by unrelated
# churn (the real-time clock, audits, checksums). Instead we alternate:
#
#   1. insert a coin  -> the coin counter INCREASES  -> keep "inc" candidates
#   2. wait, no coin   -> the RTC keeps moving but the coin counter is stable
#                        -> keep "unchanged" candidates (drops the RTC bytes)
#
# After a few rounds only the coin/credit counters survive. Coins are injected
# as timed pulses (the coin-door column is refreshed by the input port every
# frame, so a plain on/off level would not stick — a pulse is re-asserted).
#
# Usage: ./find_nvram_coins.sh   (run from src/remote_debug)
# Env overrides: ROM, ROMPATH, PORT, BINARY, ROUNDS.

PORT=${PORT:-8943}
ROM=${ROM:-taf_l7}
BINARY=${BINARY:-../../xpinmamed.x11}
ROMPATH=${ROMPATH:-$HOME/.pinmame}
ROUNDS=${ROUNDS:-5}
BASE="http://localhost:$PORT"

ROMPATH_ARGS=()
[ -d "$ROMPATH" ] && ROMPATH_ARGS=(-rompath "$ROMPATH")

echo "--------------------------------------------------"
echo "WPC NVRAM Coin Finder (value-scan method)"
echo "--------------------------------------------------"

killall -9 "$(basename "$BINARY")" 2>/dev/null
echo "1. Starting PinMAME and letting the game boot..."
"$BINARY" -headless -httpport "$PORT" -nosound "${ROMPATH_ARGS[@]}" "$ROM" > /dev/null 2>&1 &
PID=$!
sleep 10

count() { curl -s "$BASE/api/debugger/scan" | sed -n 's/.*"count": *\([0-9]*\).*/\1/p'; }

echo "2. Snapshotting the battery-backed RAM (0x0000-0x1FFF)..."
curl -s "$BASE/api/debugger/scan?cmd=new&addr=0000&size=8192" > /dev/null
echo "   candidates: $(count)"

echo "3. Isolating the coin counters over $ROUNDS rounds..."
for r in $(seq 1 "$ROUNDS"); do
    # insert a coin -> counter increases
    curl -s "$BASE/api/input?sw=1&val=1&pulse=300" > /dev/null
    sleep 1.5
    curl -s "$BASE/api/debugger/scan?cmd=filter&op=inc" > /dev/null
    # let the RTC advance without a coin -> counter unchanged
    sleep 2.5
    curl -s "$BASE/api/debugger/scan?cmd=filter&op=unchanged" > /dev/null
    echo "   round $r -> candidates: $(count)"
done

echo "4. Remaining coin/credit counter candidates:"
echo "   OFFSET | VALUE"
echo "   --------------"
curl -s "$BASE/api/debugger/scan" | python3 -c '
import json, sys
try:
    d = json.load(sys.stdin)
except Exception:
    print("   (could not parse scan result)"); sys.exit(0)
for r in d.get("results", []):
    print("   0x%04X | %d" % (r["addr"], r["val"]))
' 2>/dev/null || curl -s "$BASE/api/debugger/scan"

# Also keep a raw NVRAM dump for reference / external tooling.
curl -s "$BASE/api/debugger/nvram/dump" > nvram.bin
echo "   (raw 8KB NVRAM dump saved to nvram.bin)"

kill -9 $PID 2>/dev/null
echo "--------------------------------------------------"
echo "Done. The surviving offsets are the coin/credit counters"
echo "(on WPC, the per-chute coin audits plus a total). Set a"
echo "write watchpoint on one to see the code that updates it, e.g.:"
echo "  curl \"$BASE/api/debugger/watchpoints?cmd=add&addr=<offset>&mode=2\""
echo "--------------------------------------------------"
