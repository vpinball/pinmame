#!/bin/bash
# PinMAME Remote Debugger verification suite, Capcom part.
#
# test_suite.sh exercises the debugger on a WPC game, whose M6809 only makes
# byte accesses. This one covers a big endian 16 bit CPU (the M68306 of the
# Capcom games) with a 32 bit address space: value conditions on 16 bit
# writes and the bounds of trace ranges.
#
# Usage: ./test_suite_capcom.sh  (run from src/remote_debug)
# Environment overrides:
#   ROM      game to load             (default: bsv103)
#   ROMPATH  ROM search path          (default: ~/.pinmame if it exists)
#   PORT     HTTP port                (default: 8946)
#   BINARY   emulator binary          (default: ../../xpinmamed.x11)

PORT=${PORT:-8946}
ROM=${ROM:-bsv103}
BINARY=${BINARY:-../../xpinmamed.x11}
ROMPATH=${ROMPATH:-$HOME/.pinmame}
BASE="http://localhost:$PORT"

ROMPATH_ARGS=()
[ -d "$ROMPATH" ] && ROMPATH_ARGS=(-rompath "$ROMPATH")

echo "=================================================="
echo "PinMAME Remote Debugger Verification Suite (Capcom)"
echo "=================================================="

killall -9 "$(basename "$BINARY")" 2>/dev/null
"$BINARY" -headless -startpaused -httpport "$PORT" -nosound "${ROMPATH_ARGS[@]}" "$ROM" > suite_capcom_log.txt 2>&1 &
PID=$!
sleep 5

fail() {
    echo "  [FAIL] $1"
    shift
    for line in "$@"; do echo "  $line"; done
    kill -9 $PID 2>/dev/null
    exit 1
}

assert_contains() {
    if [[ "$1" == *"$2"* ]]; then echo "  [PASS] $3"; else fail "$3 (expected '$2')" "Body: $1"; fi
}

assert_status() {
    local code
    code=$(curl -s -o /dev/null -w "%{http_code}" "$1")
    if [ "$code" == "$2" ]; then echo "  [PASS] $3"; else fail "$3 (expected HTTP $2, got $code)"; fi
}

# wait_paused SECONDS: poll until the emulator halts; fails after SECONDS
wait_paused() {
    local i
    for i in $(seq 1 $(($1 * 10))); do
        [[ "$(curl -s "$BASE/api/info" | tr -d ' ')" == *'"paused":1'* ]] && return 0
        sleep 0.1
    done
    return 1
}

read_byte() {
    curl -s "$BASE/api/debugger/memory?addr=$1&size=1" | python3 -c "import json,sys;print(json.load(sys.stdin)['data'][0])"
}

echo "1. Info..."
INFO=$(curl -s "$BASE/api/info")
assert_contains "$INFO" "$ROM" "Game name"

echo "2. Trace range bounds (32 bit address space)..."
curl -s "$BASE/api/debugger/trace?cmd=clear" > /dev/null
assert_status "$BASE/api/debugger/trace?cmd=add&addr=30000000&len=0x10000" 200 "Trace range in NVRAM"
assert_status "$BASE/api/debugger/trace?cmd=add&addr=FFFFFFFF&len=1" 200 "Trace range at the top of the address space"
assert_status "$BASE/api/debugger/trace?cmd=add&addr=FFFFFFFF&len=2" 400 "400 on trace range that wraps"
curl -s "$BASE/api/debugger/trace?cmd=clear" > /dev/null

echo "3. Value condition on 16 bit writes..."
# find a DRAM word the game keeps writing with a changing low byte
curl -s "$BASE/api/debugger/trace?cmd=add&addr=0&len=0x10000&mode=2" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 1
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
WORD=$(curl -s "$BASE/api/debugger/trace" | python3 -c "
import json,sys,collections
n=collections.Counter(); low=collections.defaultdict(set)
for e in json.load(sys.stdin)['logs']:
    if e['len']==2:
        n[e['adr']]+=1; low[e['adr']].add(e['value']&0xFF)
w=[a for a,c in n.most_common() if len(low[a])>1]
print('%X'%w[0] if w else '')")
curl -s "$BASE/api/debugger/trace?cmd=clear" > /dev/null
[ -n "$WORD" ] || fail "no word written with a changing low byte"
echo "  [INFO] word at $WORD"
# watch the second byte of the word (its low byte, big endian): the next
# word write that changes it must halt, as it would for a byte write
LOWADR=$(printf '%X' $((0x$WORD + 1)))
OLD=$(read_byte "$LOWADR")
curl -s "$BASE/api/debugger/trace?cmd=add&addr=$WORD&len=2&mode=2" > /dev/null
curl -s "$BASE/api/debugger/watchpoints?cmd=add&addr=$LOWADR&mode=2&cond=ne&val=$(printf '%X' "$OLD")" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
wait_paused 10 || fail "Value-cond WP on the low byte of a word did not halt within 10 s"
echo "  [PASS] Value-cond WP on the low byte of a word write halts"
M=$(read_byte "$LOWADR")
LAST=$(curl -s "$BASE/api/debugger/trace" | python3 -c "import json,sys;e=json.load(sys.stdin)['logs'][-1];print(e['len'],e['value'])")
read -r LEN VAL <<< "$LAST"
[ "$LEN" == "2" ] && echo "  [PASS] Halted on a word write" || fail "halted on a write of $LEN bytes"
[ "$M" != "$OLD" ] && [ "$M" == "$((VAL & 0xFF))" ] && echo "  [PASS] Low byte holds the low byte of the word written" \
    || fail "low byte is $M (was $OLD), word written $VAL"
curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/trace?cmd=clear" > /dev/null

echo "=================================================="
echo "ALL TESTS PASSED"
echo "=================================================="

kill -9 $PID 2>/dev/null
exit 0
