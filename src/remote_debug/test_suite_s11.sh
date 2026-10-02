#!/bin/bash
# PinMAME Remote Debugger verification suite, Williams System 11 part.
#
# test_suite.sh exercises the debugger on a WPC game. This one covers what
# differs on another generation: a M6808 main CPU, sequential switch and lamp
# numbers, the operator buttons of System 11 and its NVRAM.
#
# Usage: ./test_suite_s11.sh  (run from src/remote_debug)
# Environment overrides:
#   ROM      game to load             (default: f14_l1)
#   ROMPATH  ROM search path          (default: ~/.pinmame if it exists)
#   PORT     HTTP port                (default: 8945)
#   BINARY   emulator binary          (default: ../../xpinmamed.x11)

PORT=${PORT:-8945}
ROM=${ROM:-f14_l1}
BINARY=${BINARY:-../../xpinmamed.x11}
ROMPATH=${ROMPATH:-$HOME/.pinmame}
BASE="http://localhost:$PORT"

ROMPATH_ARGS=()
[ -d "$ROMPATH" ] && ROMPATH_ARGS=(-rompath "$ROMPATH")

echo "=================================================="
echo "PinMAME Remote Debugger Verification Suite (System 11)"
echo "=================================================="

killall -9 "$(basename "$BINARY")" 2>/dev/null
"$BINARY" -headless -startpaused -httpport "$PORT" -nosound "${ROMPATH_ARGS[@]}" "$ROM" > suite_s11_log.txt 2>&1 &
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

assert_not_contains() {
    if [[ "$1" != *"$2"* ]]; then echo "  [PASS] $3"; else fail "$3 (unexpected '$2')" "Body: $1"; fi
}

assert_status() {
    local code
    code=$(curl -s -o /dev/null -w "%{http_code}" "$1")
    if [ "$code" == "$2" ]; then echo "  [PASS] $3"; else fail "$3 (expected HTTP $2, got $code)"; fi
}

# program counter of the main CPU
main_pc() {
    curl -s "$BASE/api/debugger/state" | tr -d ' ' | grep -oE '"pc":[0-9]+' | head -n 1 | cut -d: -f2
}

echo "1. Info & main CPU..."
INFO=$(curl -s "$BASE/api/info")
assert_contains "$INFO" "$ROM" "Game name"
CPU0=$(curl -s "$BASE/api/debugger/state" | tr -d ' ' | cut -d'}' -f1)
assert_contains "$CPU0" '"name":"M6808"' "Main CPU is a M6808"
assert_contains "$CPU0" '"a":' "M6800 family register A"
assert_contains "$CPU0" '"x":' "M6800 family register X"
assert_contains "$CPU0" '"cc":' "M6800 family register CC"
assert_not_contains "$CPU0" '"y":' "No M6809 registers on a M6808"

echo "2. Register write by name..."
curl -s "$BASE/api/debugger/state/write?reg=X&val=1234" > /dev/null
CPU0=$(curl -s "$BASE/api/debugger/state" | tr -d ' ' | cut -d'}' -f1)
assert_contains "$CPU0" '"x":4660' "Register X written"

echo "3. Switch and lamp numbering..."
SW=$(curl -s "$BASE/api/switches" | tr -d ' ')
assert_contains "$SW" '"num":1,"col":1,"row":1' "Switch 1 is column 1, row 1"
assert_contains "$SW" '"num":64,"col":8,"row":8' "Switch 64 is column 8, row 8"
assert_contains "$SW" '"num":-7,"col":0,"row":1' "Dedicated column counts from -7"
LAMPS=$(curl -s "$BASE/api/lamps" | tr -d ' ')
assert_contains "$LAMPS" '"num":1,"col":1,"row":1' "Lamp 1 is column 1, row 1"
assert_contains "$LAMPS" '"num":64,"col":8,"row":8' "Lamp 64 is column 8, row 8"

echo "4. Switch by its number..."
curl -s "$BASE/api/input?sw=12&val=1" > /dev/null
SW=$(curl -s "$BASE/api/switches" | tr -d ' ')
assert_contains "$SW" '"num":12,"col":2,"row":4,"active":1' "Switch 12 set (column 2, row 4)"
curl -s "$BASE/api/input?sw=12&val=0" > /dev/null

echo "5. Operator buttons..."
BTN=$(curl -s "$BASE/api/input/buttons" | tr -d ' ')
assert_contains "$BTN" '"mask":256,"name":"Advance","toggle":0' "Advance listed"
assert_contains "$BTN" '"mask":512,"name":"Up/Down","toggle":1' "Up/Down is a toggle"
assert_contains "$BTN" '"name":"Coin1"' "Coin 1 listed"
# the buttons reach the switch matrix through the driver, once per frame
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 2
curl -s "$BASE/api/input/button?mask=100&val=1&pulse=600" > /dev/null
sleep 0.2
SW=$(curl -s "$BASE/api/switches" | tr -d ' ')
assert_contains "$SW" '"num":-7,"col":0,"row":1,"active":1' "Advance closes its switch"
sleep 1
SW=$(curl -s "$BASE/api/switches" | tr -d ' ')
assert_contains "$SW" '"num":-7,"col":0,"row":1,"active":0' "Advance released after the pulse"
curl -s "$BASE/api/input/button?mask=200&val=1" > /dev/null
sleep 0.3
BTN=$(curl -s "$BASE/api/input/buttons" | tr -d ' ')
assert_contains "$BTN" '"mask":512,"name":"Up/Down","toggle":1,"active":1' "Up/Down switched on"
curl -s "$BASE/api/input/button?mask=200&val=0" > /dev/null

echo "6. NVRAM..."
NV=$(curl -s "$BASE/api/debugger/nvram" | tr -d ' ')
assert_contains "$NV" '{"size":2048,"cpu":0,"addr":0}' "NVRAM is 0000-07FF of the main CPU"
NV_DUMP=$(curl -s "$BASE/api/debugger/nvram/dump" | wc -c)
[ "$NV_DUMP" == "2048" ] && echo "  [PASS] NVRAM dump ($NV_DUMP bytes)" || fail "NVRAM dump is $NV_DUMP bytes, expected 2048"

echo "7. Memory of a CPU without memory map..."
# sound is off, so the sound CPUs are not set up: must not take the emulator down
MEM1=$(curl -s "$BASE/api/debugger/memory?addr=8000&size=4&cpu=1" | tr -d ' ')
assert_contains "$MEM1" '"data":[0,0,0,0]' "Sound CPU memory reads as zero"
assert_status "$BASE/api/info" 200 "Emulator still alive"

echo "8. Breakpoint conditions..."
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
assert_status "$BASE/api/debugger/breakpoints?cmd=add&addr=F000&cond=A==FF" 200 "Condition on A accepted"
assert_status "$BASE/api/debugger/breakpoints?cmd=add&addr=F000&cond=Y==FF" 400 "Condition on Y refused (no such register)"
curl -s "$BASE/api/debugger/breakpoints?cmd=clear" > /dev/null

echo "9. Save states..."
curl -s "$BASE/api/debugger/memory/write?addr=1F00&data=0102" > /dev/null
SAVE=$(curl -s "$BASE/api/debugger/savestate?cmd=save&slot=s11")
assert_contains "$SAVE" '"status": "ok"' "State saved"
PC_SAVED=$(main_pc)
curl -s "$BASE/api/debugger/memory/write?addr=1F00&data=AABB" > /dev/null
DIFF=$(curl -s "$BASE/api/debugger/savestate/diff?a=s11" | tr -d ' ')
assert_contains "$DIFF" '"addr":7936,"a":1,"b":170' "Save-state diff finds changed byte"
curl -s "$BASE/api/debugger/control?cmd=step" > /dev/null
sleep 0.5
LOAD=$(curl -s "$BASE/api/debugger/savestate?cmd=load&slot=s11")
assert_contains "$LOAD" '"status": "ok"' "State loaded"
MEM=$(curl -s "$BASE/api/debugger/memory?addr=1F00&size=2" | tr -d ' ')
assert_contains "$MEM" '"data":[1,2]' "RAM restored"
[ "$(main_pc)" == "$PC_SAVED" ] && echo "  [PASS] PC restored" || fail "PC is $(main_pc), saved $PC_SAVED"

echo "10. Reset, exact halt and single step..."
curl -s "$BASE/api/debugger/control?cmd=reset" > /dev/null
sleep 1
RESET_PC=$(main_pc)
VEC=$(curl -s "$BASE/api/debugger/memory?addr=FFFE&size=2" | tr -d ' ' | grep -oE '"data":\[[0-9,]*\]' | tr -d '"data:[]')
[ "$RESET_PC" == "$(( ${VEC%,*} * 256 + ${VEC#*,} ))" ] && echo "  [PASS] Reset while paused halts at the reset vector" || fail "PC after reset is $RESET_PC, vector bytes $VEC"
# the addresses of the next instructions, from the disassembler
NEXT=($(curl -s "$BASE/api/debugger/dasm?addr=$(printf %X "$RESET_PC")&lines=4" | grep -oE '"addr": [0-9]+' | cut -d' ' -f2))
curl -s "$BASE/api/debugger/control?cmd=step" > /dev/null
sleep 0.5
[ "$(main_pc)" == "${NEXT[1]}" ] && echo "  [PASS] Step executes exactly one instruction" || fail "PC after step is $(main_pc), expected ${NEXT[1]}"
curl -s "$BASE/api/debugger/breakpoints?cmd=add&addr=$(printf %X "${NEXT[3]}")" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 1
[ "$(main_pc)" == "${NEXT[3]}" ] && echo "  [PASS] Breakpoint halts in front of its instruction" || fail "PC at breakpoint is $(main_pc), expected ${NEXT[3]}"
curl -s "$BASE/api/debugger/breakpoints?cmd=clear" > /dev/null

echo "11. Callstack, step over and step out..."
# let the game finish its start-up tests, then step until the main CPU stands
# in front of a subroutine call
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 4
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
sleep 0.5
CALL_PC=""
for i in $(seq 1 2000); do
    PC=$(main_pc)
    TEXT=$(curl -s "$BASE/api/debugger/dasm?addr=$(printf %X "$PC")&lines=2")
    if [[ "$TEXT" == *'"text": "jsr'* ]] && [ "$(echo "$TEXT" | grep -oE '"text": "[a-z]+' | head -n 1)" == '"text": "jsr' ]; then
        CALL_PC=$PC
        break
    fi
    curl -s "$BASE/api/debugger/control?cmd=step" > /dev/null
done
[ -n "$CALL_PC" ] && echo "  [PASS] Reached a subroutine call" || fail "no jsr found while stepping"
AFTER=$(echo "$TEXT" | grep -oE '"addr": [0-9]+' | sed -n 2p | cut -d' ' -f2)
DEPTH=$(curl -s "$BASE/api/debugger/callstack" | grep -o '"caller"' | wc -l)
curl -s "$BASE/api/debugger/control?cmd=step" > /dev/null
sleep 0.5
STACK=$(curl -s "$BASE/api/debugger/callstack" | tr -d ' ')
assert_contains "$STACK" "\"caller\":$CALL_PC," "Call recorded on the callstack"
[ "$(echo "$STACK" | grep -o '"caller"' | wc -l)" == "$((DEPTH + 1))" ] && echo "  [PASS] Callstack one frame deeper" || fail "callstack depth did not grow by one" "Body: $STACK"
curl -s "$BASE/api/debugger/control?cmd=stepout" > /dev/null
sleep 1
[ "$(main_pc)" == "$AFTER" ] && echo "  [PASS] Step out returns behind the call" || fail "PC after step out is $(main_pc), expected $AFTER"
[ "$(curl -s "$BASE/api/debugger/callstack" | grep -o '"caller"' | wc -l)" == "$DEPTH" ] && echo "  [PASS] Frame gone after the return" || fail "callstack depth not back to $DEPTH"

echo "=================================================="
echo "ALL TESTS PASSED"
echo "=================================================="

kill -9 $PID 2>/dev/null
exit 0
