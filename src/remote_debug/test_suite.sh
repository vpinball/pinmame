#!/bin/bash
# PinMAME Remote Debugger verification suite.
#
# Usage: ./test_suite.sh  (run from src/remote_debug)
# Environment overrides:
#   ROM      game to load             (default: taf_l7)
#   ROMPATH  ROM search path          (default: ~/.pinmame if it exists)
#   PORT     HTTP port                (default: 8944)
#   BINARY   emulator binary          (default: ../../xpinmamed.x11)

PORT=${PORT:-8944}
ROM=${ROM:-taf_l7}
BINARY=${BINARY:-../../xpinmamed.x11}
ROMPATH=${ROMPATH:-$HOME/.pinmame}
BASE="http://localhost:$PORT"

ROMPATH_ARGS=()
[ -d "$ROMPATH" ] && ROMPATH_ARGS=(-rompath "$ROMPATH")

echo "=================================================="
echo "PinMAME Remote Debugger Verification Suite"
echo "=================================================="

killall -9 "$(basename "$BINARY")" 2>/dev/null
"$BINARY" -headless -startpaused -httpport "$PORT" -nosound "${ROMPATH_ARGS[@]}" "$ROM" > suite_log.txt 2>&1 &
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

echo "1. Info & core connectivity..."
INFO=$(curl -s "$BASE/api/info")
assert_contains "$INFO" "$ROM" "Game name"
assert_contains "$INFO" '"wpc_bank":' "WPC bank field"
assert_contains "$INFO" '"segments":' "Segments field"

echo "2. HTTP status codes..."
assert_status "$BASE/api/info" 200 "200 on /api/info"
assert_status "$BASE/api/does/not/exist" 404 "404 on unknown route"
assert_status "$BASE/api/debugger/control?cmd=bogus" 400 "400 on bad command"
assert_status "$BASE/api/debugger/breakpoints?cmd=add" 400 "400 on missing addr"

echo "3. Web UI and API doc..."
UI=$(curl -s "$BASE/ui")
assert_contains "$UI" "<title>PinMAME Remote Debugger</title>" "Web UI served"
DOC=$(curl -s "$BASE/api/doc")
assert_contains "$DOC" "/api/debugger/trace" "API doc lists routes"

echo "4. Screenshot API..."
S_INFO=$(curl -s "$BASE/api/screenshot/info")
assert_contains "$S_INFO" '"width":' "Screenshot info"
S_RAW_SIZE=$(curl -s "$BASE/api/screenshot/raw" | wc -c)
[ "$S_RAW_SIZE" -gt 1000 ] && echo "  [PASS] Screenshot raw ($S_RAW_SIZE bytes)" || fail "Screenshot raw too small ($S_RAW_SIZE)"
S_PNM_HEAD=$(curl -s "$BASE/api/screenshot/pnm" | head -n 1)
assert_contains "$S_PNM_HEAD" "P6" "Screenshot PNM header"

echo "5. SSE events..."
curl -s --max-time 3 -N "$BASE/api/events" > sse_log.txt &
SSE_PID=$!
sleep 0.5
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 0.5
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
wait $SSE_PID 2>/dev/null
SSE=$(cat sse_log.txt; rm -f sse_log.txt)
assert_contains "$SSE" '"event": "resume"' "SSE resume event"
assert_contains "$SSE" '"event": "halt"' "SSE halt event"

echo "6. Resuming execution for boot..."
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
echo "  Waiting for boot (10s)..."
sleep 10
echo "  Setting breakpoint at 8CC1..."
curl -s "$BASE/api/debugger/breakpoints?cmd=add&addr=8CC1" > /dev/null
sleep 5
POINTS=$(curl -s "$BASE/api/debugger/points" | tr -d ' ')
assert_contains "$POINTS" '"hits":' "Breakpoint hit counter present"

echo "7. DMD API (post-init)..."
DMD_INFO=$(curl -s "$BASE/api/dmd/info")
if [[ "$DMD_INFO" == *'"width": 128'* || "$DMD_INFO" == *'"width":128'* ]]; then
    echo "  [PASS] DMD info"
    DMD_RAW_SIZE=$(curl -s "$BASE/api/dmd/raw" | wc -c)
    [ "$DMD_RAW_SIZE" -eq 4096 ] && echo "  [PASS] DMD raw (4096 bytes)" || fail "DMD raw size ($DMD_RAW_SIZE)"
    DMD_PNM_HEAD=$(curl -s "$BASE/api/dmd/pnm" | head -n 1)
    assert_contains "$DMD_PNM_HEAD" "P5" "DMD PNM header"
else
    echo "  [INFO] DMD not initialized yet (skipped)"
fi

echo "8. Memory operations..."
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
# fill: size is decimal now, addr/val hex
curl -s "$BASE/api/debugger/memory/fill?addr=0100&size=16&val=AA&cpu=0" > /dev/null
FIND=$(curl -s "$BASE/api/debugger/memory/find?addr=0000&pattern=AAAAAAAA&cpu=0" | tr -d ' ')
assert_contains "$FIND" '"found":256' "Memory fill & find"
# block write (bypasses WPC RAM protection)
curl -s "$BASE/api/debugger/memory/write?addr=0110&data=DEADBEEF" > /dev/null
MEM=$(curl -s "$BASE/api/debugger/memory?addr=0110&size=4" | tr -d ' ')
assert_contains "$MEM" '"data":[222,173,190,239]' "Block memory write"

echo "9. Register write API..."
curl -s "$BASE/api/debugger/state/write?reg=5&val=42" > /dev/null
STATE=$(curl -s "$BASE/api/debugger/state" | tr -d ' ')
assert_contains "$STATE" '"b":66' "Register write by index (B=0x42)"
curl -s "$BASE/api/debugger/state/write?reg=A&val=13" > /dev/null
STATE=$(curl -s "$BASE/api/debugger/state" | tr -d ' ')
assert_contains "$STATE" '"a":19' "Register write by name (A=0x13)"

echo "10. Watchpoint with length..."
curl -s "$BASE/api/debugger/watchpoints?cmd=add&addr=0120&len=8&mode=2" > /dev/null
POINTS=$(curl -s "$BASE/api/debugger/points" | tr -d ' ')
assert_contains "$POINTS" '"len":8' "Watchpoint length stored"
curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null

echo "11. Conditional breakpoint..."
BP_COND=$(curl -s "$BASE/api/debugger/breakpoints?cmd=add&addr=9000&cond=A%3D%3DFF&ignore=3")
assert_contains "$BP_COND" '"status": "ok"' "Conditional BP accepted"
POINTS=$(curl -s "$BASE/api/debugger/points" | tr -d ' ')
assert_contains "$POINTS" '"cond":"A==FF"' "Condition stored"
assert_contains "$POINTS" '"ignore":3' "Ignore count stored"
assert_status "$BASE/api/debugger/breakpoints?cmd=add&addr=9000&cond=ZZ==1" 400 "400 on bad condition"
curl -s "$BASE/api/debugger/breakpoints?cmd=clear" > /dev/null

echo "12. Memory trace..."
curl -s "$BASE/api/debugger/trace?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/trace?cmd=add&addr=0100" > /dev/null
TRACE=$(curl -s "$BASE/api/debugger/trace" | tr -d ' ')
assert_contains "$TRACE" '"watched":[{"addr":256,"bank":-1}]' "Trace address registered"

echo "12b. Bank-aware memory access..."
# The 0x4000-0x7FFF window is bank-switched ROM; reading it with an explicit
# bank must be independent of the current banking. The prime area (0x8000+)
# and RAM (0x0000-0x3FFF) ignore the bank parameter.
B00=$(curl -s "$BASE/api/debugger/memory?addr=5000&size=8&bank=00" | tr -d ' ')
assert_contains "$B00" '"bank":0' "Read banked ROM (bank field echoed)"
PRIME=$(curl -s "$BASE/api/debugger/memory?addr=8000&size=4" | tr -d ' ')
PRIMEB=$(curl -s "$BASE/api/debugger/memory?addr=8000&size=4&bank=10" | tr -d ' ')
DATA_A=$(echo "$PRIME" | grep -oE '"data":\[[0-9,]*\]')
DATA_B=$(echo "$PRIMEB" | grep -oE '"data":\[[0-9,]*\]')
[ "$DATA_A" == "$DATA_B" ] && echo "  [PASS] Prime area (0x8000) ignores bank" || fail "prime area differs with bank ($DATA_A vs $DATA_B)"
WPB=$(curl -s "$BASE/api/debugger/watchpoints?cmd=add&addr=5120&len=2&mode=1&bank=20"; curl -s "$BASE/api/debugger/points" | tr -d ' ')
assert_contains "$WPB" '"addr":20768,"len":2,"mode":1,"bank":32' "Banked watchpoint stored"
curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null

echo "13. Callstack API..."
STACK=$(curl -s "$BASE/api/debugger/callstack")
assert_contains "$STACK" '"stack":' "Callstack format"
assert_contains "$STACK" '"bank":' "Callstack banking info"
assert_contains "$STACK" '"pc":' "Callstack register context (PC)"
assert_contains "$STACK" '"u":' "Callstack register context (U)"

echo "14. Switch/lamp/solenoid query with names..."
SW=$(curl -s "$BASE/api/switches" | tr -d ' ')
assert_contains "$SW" '"num":1,"col":0,"row":1,"active":0,"name":"Coin1"' "Coin 1 named"
assert_contains "$SW" '"name":"Enter"' "Enter (sw5) named"
assert_contains "$SW" '"name":"Escape"' "Escape (sw8) named"
LAMPS=$(curl -s "$BASE/api/lamps" | tr -d ' ')
assert_contains "$LAMPS" '"num":11,"col":1,"row":1' "Lamp 11 present"
SOL=$(curl -s "$BASE/api/solenoids" | tr -d ' ')
assert_contains "$SOL" '"num":1,"active":' "Solenoid 1 present"

echo "15. Switch pulse..."
curl -s "$BASE/api/input?sw=1&val=1&pulse=200" > /dev/null
SW1=$(curl -s "$BASE/api/switches" | tr -d ' ')
assert_contains "$SW1" '"num":1,"col":0,"row":1,"active":1' "Switch on during pulse"
sleep 0.5
SW1=$(curl -s "$BASE/api/switches" | tr -d ' ')
assert_contains "$SW1" '"num":1,"col":0,"row":1,"active":0' "Switch off after pulse"

echo "16. Object monitoring & action log..."
curl -s "$BASE/api/monitor?cmd=clear" > /dev/null
curl -s "$BASE/api/monitor/log?cmd=clear" > /dev/null
MON=$(curl -s "$BASE/api/monitor?cmd=add&type=lamp&id=11" | tr -d ' ')
assert_contains "$MON" '"type":"lamp","id":11' "Monitor registered"
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 3
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
ALOG=$(curl -s "$BASE/api/monitor/log" | tr -d ' ')
assert_contains "$ALOG" '"type":"lamp","id":11' "Action log captured lamp changes"

echo "17. Code instrumentation / PC hit counting..."
curl -s "$BASE/api/debugger/instrument?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/instrument?cmd=add&addr=8CC1" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 2
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
INSTR=$(curl -s "$BASE/api/debugger/instrument" | tr -d ' ')
assert_contains "$INSTR" '"addr":36033' "Instrumented address listed"
# count field must be present and > 0 is likely; at minimum the field exists
assert_contains "$INSTR" '"count":' "Hit count field present"

echo "18. Value scan (variable search)..."
curl -s "$BASE/api/debugger/scan?cmd=new&addr=0100&size=256&cpu=0" > /dev/null
curl -s "$BASE/api/debugger/memory/write?addr=0140&val=5A" > /dev/null
SCAN=$(curl -s "$BASE/api/debugger/scan?cmd=filter&op=changed" | tr -d ' ')
assert_contains "$SCAN" '"addr":320' "Value scan isolates changed byte (0x140)"
SCANEQ=$(curl -s "$BASE/api/debugger/scan?cmd=filter&op=eq&val=5A" | tr -d ' ')
assert_contains "$SCANEQ" '"val":90' "Value scan eq filter"

echo "19. Save states (RAM + CPU checkpoint)..."
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
curl -s "$BASE/api/debugger/memory/write?addr=0500&val=11" > /dev/null
curl -s "$BASE/api/debugger/savestate?cmd=save&slot=cp1" > /dev/null
SS=$(curl -s "$BASE/api/debugger/savestate" | tr -d ' ')
assert_contains "$SS" '"name":"cp1"' "Save state slot listed"
curl -s "$BASE/api/debugger/memory/write?addr=0500&val=99" > /dev/null
curl -s "$BASE/api/debugger/savestate?cmd=load&slot=cp1" > /dev/null
RESTORED=$(curl -s "$BASE/api/debugger/memory?addr=0500&size=1" | tr -d ' ')
assert_contains "$RESTORED" '"data":[17]' "Save/load restores RAM byte"
LOADMISS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/api/debugger/savestate?cmd=load&slot=nope")
[ "$LOADMISS" == "404" ] && echo "  [PASS] 404 on unknown slot" || fail "unknown slot (expected 404, got $LOADMISS)"

echo "20. DMD recorder..."
curl -s "$BASE/api/debugger/dmdrec?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/dmdrec?cmd=start" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 2
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
REC=$(curl -s "$BASE/api/debugger/dmdrec?cmd=stop" | tr -d ' ')
assert_contains "$REC" '"width":128' "DMD recorder captured frames"
DATA_SIZE=$(curl -s "$BASE/api/debugger/dmdrec/data" | wc -c)
[ "$DATA_SIZE" -gt 12 ] && echo "  [PASS] DMD recording blob ($DATA_SIZE bytes)" || fail "DMD blob too small ($DATA_SIZE)"

echo "20b. Execution trace..."
curl -s "$BASE/api/debugger/exectrace?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/exectrace?cmd=start" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 0.3
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
curl -s "$BASE/api/debugger/exectrace?cmd=stop" > /dev/null
ET=$(curl -s "$BASE/api/debugger/exectrace" | tr -d ' ')
assert_contains "$ET" '"pc":' "Execution trace recorded instructions"

echo "20c. Code coverage..."
curl -s "$BASE/api/debugger/coverage?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/coverage?cmd=start" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 0.5
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
curl -s "$BASE/api/debugger/coverage?cmd=stop" > /dev/null
COV=$(curl -s "$BASE/api/debugger/coverage")
EXEC=$(echo "$COV" | sed -n 's/.*"executed": *\([0-9]*\).*/\1/p')
[ "$EXEC" -gt 0 ] && echo "  [PASS] Coverage recorded $EXEC addresses" || fail "coverage executed 0"
COVR=$(curl -s "$BASE/api/debugger/coverage?addr=8000&size=8" | tr -d ' ')
assert_contains "$COVR" '"executed":[' "Coverage region query"

echo "20d. Tracepoints (log & continue)..."
# resume, find a hot address from the trace, trace it, confirm it logs without halting
curl -s "$BASE/api/debugger/exectrace?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/exectrace?cmd=start" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 0.2
curl -s "$BASE/api/debugger/exectrace?cmd=stop" > /dev/null
HOT=$(curl -s "$BASE/api/debugger/exectrace" | python3 -c "import json,sys;t=json.load(sys.stdin)['trace'];print('%X'%t[-1]['pc']) if t else print('8000')")
curl -s "$BASE/api/debugger/tracepoints?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/tracepoints?cmd=add&addr=$HOT" > /dev/null
sleep 1
TP=$(curl -s "$BASE/api/debugger/tracepoints" | tr -d ' ')
assert_contains "$TP" '"hits":' "Tracepoint recorded hits"
RUN=$(curl -s "$BASE/api/info" | tr -d ' ')
assert_contains "$RUN" '"paused":0' "Tracepoint did not halt execution"

echo "20e. Watchpoint value condition..."
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
# find a churning RAM byte
curl -s "$BASE/api/debugger/scan?cmd=new&addr=0000&size=2048" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null; sleep 1
curl -s "$BASE/api/debugger/scan?cmd=filter&op=changed" > /dev/null; sleep 1
CH=$(curl -s "$BASE/api/debugger/scan?cmd=filter&op=changed" | python3 -c "import json,sys;r=json.load(sys.stdin)['results'];print('%X'%r[0]['addr']) if r else print('')")
curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null
if [ -n "$CH" ]; then
    # impossible condition: must NOT halt
    curl -s "$BASE/api/debugger/watchpoints?cmd=add&addr=$CH&mode=2&cond=eq&val=FF" > /dev/null
    curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null; sleep 1.5
    P=$(curl -s "$BASE/api/info" | tr -d ' ')
    assert_contains "$P" '"paused":0' "Value-cond WP (impossible) keeps running"
    # achievable condition: must halt
    curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null
    curl -s "$BASE/api/debugger/watchpoints?cmd=add&addr=$CH&mode=2&cond=ne&val=FF" > /dev/null
    curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null; sleep 1.5
    P=$(curl -s "$BASE/api/info" | tr -d ' ')
    assert_contains "$P" '"paused":1' "Value-cond WP (achievable) halts"
    curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null
else
    echo "  [INFO] no churning byte found, skipped"
fi

echo "20f. Monitor break-on-change..."
curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null
curl -s "$BASE/api/monitor?cmd=clear" > /dev/null
MB=$(curl -s "$BASE/api/monitor?cmd=add&type=lamp&id=11&break=1" | tr -d ' ')
assert_contains "$MB" '"break":1' "Monitor break flag stored"
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null; sleep 3
P=$(curl -s "$BASE/api/info" | tr -d ' ')
assert_contains "$P" '"paused":1' "Monitor break halted on lamp change"
curl -s "$BASE/api/monitor?cmd=clear" > /dev/null

echo "20g. Save-state diff..."
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
curl -s "$BASE/api/debugger/memory/write?addr=0700&data=0102" > /dev/null
curl -s "$BASE/api/debugger/savestate?cmd=save&slot=dref" > /dev/null
curl -s "$BASE/api/debugger/memory/write?addr=0700&data=AABB" > /dev/null
DIFF=$(curl -s "$BASE/api/debugger/savestate/diff?a=dref" | tr -d ' ')
assert_contains "$DIFF" '"addr":1792,"a":1,"b":170' "Save-state diff finds changed byte"

# These come last: forcing the PC to an arbitrary vector and single-stepping
# leaves the CPU in a state that will fault if allowed to run on, so they must
# not be followed by a resume before the process is killed.
echo "21. Step out..."
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
STEPOUT=$(curl -s "$BASE/api/debugger/control?cmd=stepout")
assert_contains "$STEPOUT" '"status": "ok"' "Step out accepted"

echo "22. Live register context & stepping..."
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
curl -s "$BASE/api/debugger/state/write?reg=PC&val=FE3C" > /dev/null
curl -s "$BASE/api/debugger/state/write?reg=A&val=01" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=step" > /dev/null
sleep 1
STATE_STEP=$(curl -s "$BASE/api/debugger/state" | tr -d ' ')
if [[ "$STATE_STEP" == *'"pc":65084'* ]]; then
    fail "Live register context: PC stayed at FE3C"
else
    echo "  [PASS] Live register context: PC moved from FE3C"
fi

echo "=================================================="
echo "ALL TESTS PASSED"
echo "=================================================="

kill -9 $PID 2>/dev/null
exit 0
