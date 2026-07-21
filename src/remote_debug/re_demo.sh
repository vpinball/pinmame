#!/bin/bash
# Reverse-engineering demo: shows the tracing/coverage/tracepoint tools on a
# running WPC game, chaining them into a realistic "how does this work" flow.
#
#   1. Coverage    - which code ran during a slice of attract mode
#   2. Exec trace  - the exact instruction stream leading up to a halt
#   3. Tracepoint  - log registers at a hot routine without stopping the game
#   4. Watchpoint with value condition - catch the write that sets a value
#   5. Monitor break-on-change - stop the moment a lamp/solenoid toggles
#   6. Save-state diff - what RAM changed between two checkpoints
#
# Usage: ./re_demo.sh   (run from src/remote_debug)
# Env overrides: ROM, ROMPATH, PORT, BINARY.

PORT=${PORT:-8955}
ROM=${ROM:-taf_l7}
BINARY=${BINARY:-../../xpinmamed.x11}
ROMPATH=${ROMPATH:-$HOME/.pinmame}
BASE="http://localhost:$PORT"

ROMPATH_ARGS=()
[ -d "$ROMPATH" ] && ROMPATH_ARGS=(-rompath "$ROMPATH")

# small JSON field extractor (python3 if present, else sed)
jget() { python3 -c "import json,sys;print(json.load(sys.stdin)$1)" 2>/dev/null; }

echo "=================================================="
echo "PinMAME Remote Debugger - RE tooling demo ($ROM)"
echo "=================================================="

killall -9 "$(basename "$BINARY")" 2>/dev/null
"$BINARY" -headless -httpport "$PORT" -nosound "${ROMPATH_ARGS[@]}" "$ROM" > /dev/null 2>&1 &
PID=$!
trap 'kill -9 $PID 2>/dev/null' EXIT
echo "Booting..."; sleep 10

echo
echo "1. CODE COVERAGE -- record which addresses execute in 2s of attract mode"
curl -s "$BASE/api/debugger/coverage?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/coverage?cmd=start" > /dev/null
sleep 2
curl -s "$BASE/api/debugger/coverage?cmd=stop" > /dev/null
EXECUTED=$(curl -s "$BASE/api/debugger/coverage" | jget "['executed']")
echo "   -> $EXECUTED distinct code addresses executed"
echo "   (in the web UI, tick 'mark executed in disassembler' to see them highlighted)"

echo
echo "2. EXECUTION TRACE -- capture the instruction stream, then show the tail"
curl -s "$BASE/api/debugger/exectrace?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/exectrace?cmd=start" > /dev/null
sleep 1
curl -s "$BASE/api/debugger/exectrace?cmd=stop" > /dev/null
echo "   last 8 executed instructions (bank:pc  A B X):"
curl -s "$BASE/api/debugger/exectrace" | python3 -c '
import json,sys
for e in json.load(sys.stdin)["trace"][-8:]:
    bank = "--" if e["bank"]==-1 else "%02X"%e["bank"]
    print("     %s:%04X  A=%02X B=%02X X=%04X" % (bank, e["pc"], e["a"], e["b"], e["x"]))
' 2>/dev/null
HOT=$(curl -s "$BASE/api/debugger/exectrace" | python3 -c "import json,sys;t=json.load(sys.stdin)['trace'];print('%X'%t[-1]['pc'])" 2>/dev/null)

echo
echo "3. TRACEPOINT -- log registers every time PC hits 0x$HOT, WITHOUT halting"
curl -s "$BASE/api/debugger/tracepoints?cmd=clear" > /dev/null
curl -s "$BASE/api/debugger/tracepoints?cmd=add&addr=$HOT" > /dev/null
sleep 1
HITS=$(curl -s "$BASE/api/debugger/tracepoints" | jget "['points'][0]['hits']")
PAUSED=$(curl -s "$BASE/api/info" | jget "['paused']")
echo "   -> logged $HITS hits; emulator still running (paused=$PAUSED)"
curl -s "$BASE/api/debugger/tracepoints?cmd=clear" > /dev/null

echo
echo "4. VALUE-CONDITION WATCHPOINT -- find a changing RAM byte, then catch the"
echo "   write that sets it to a specific value"
curl -s "$BASE/api/debugger/scan?cmd=new&addr=0000&size=2048" > /dev/null; sleep 1
curl -s "$BASE/api/debugger/scan?cmd=filter&op=changed" > /dev/null; sleep 1
VAR=$(curl -s "$BASE/api/debugger/scan?cmd=filter&op=changed" | python3 -c "import json,sys;r=json.load(sys.stdin)['results'];print('%X'%r[0]['addr']) if r else print('')" 2>/dev/null)
if [ -n "$VAR" ]; then
    echo "   watching 0x$VAR for a write of any non-0xFF value..."
    curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null
    curl -s "$BASE/api/debugger/watchpoints?cmd=add&addr=$VAR&mode=2&cond=ne&val=FF" > /dev/null
    curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
    sleep 1.5
    PAUSED=$(curl -s "$BASE/api/info" | jget "['paused']")
    PC=$(curl -s "$BASE/api/debugger/state" | jget "['cpus'][0]['pc']")
    echo "   -> halted=$PAUSED at PC=$PC (the instruction that wrote 0x$VAR)"
    echo "   callstack:"
    curl -s "$BASE/api/debugger/callstack" | python3 -c '
import json,sys
for e in json.load(sys.stdin).get("stack",[])[-4:]:
    print("     %04X -> %04X (bank %02X)" % (e["caller"], e["receiver"], e["bank"]))
' 2>/dev/null
    curl -s "$BASE/api/debugger/watchpoints?cmd=clear" > /dev/null
fi

echo
echo "5. MONITOR BREAK-ON-CHANGE -- stop the game the moment lamp 11 toggles"
curl -s "$BASE/api/monitor?cmd=clear" > /dev/null
curl -s "$BASE/api/monitor?cmd=add&type=lamp&id=11&break=1" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 3
PAUSED=$(curl -s "$BASE/api/info" | jget "['paused']")
echo "   -> halted=$PAUSED after lamp 11 changed state"
curl -s "$BASE/api/monitor?cmd=clear" > /dev/null

echo
echo "6. SAVE-STATE DIFF -- checkpoint, run a moment, see what RAM changed"
curl -s "$BASE/api/debugger/savestate?cmd=save&slot=demo" > /dev/null
curl -s "$BASE/api/debugger/control?cmd=resume" > /dev/null
sleep 1
curl -s "$BASE/api/debugger/control?cmd=pause" > /dev/null
DIFFCOUNT=$(curl -s "$BASE/api/debugger/savestate/diff?a=demo" | jget "['count']")
echo "   -> $DIFFCOUNT RAM bytes changed vs the checkpoint; first few:"
curl -s "$BASE/api/debugger/savestate/diff?a=demo" | python3 -c '
import json,sys
for x in json.load(sys.stdin)["diffs"][:6]:
    print("     0x%04X: %02X -> %02X" % (x["addr"], x["a"], x["b"]))
' 2>/dev/null

echo
echo "=================================================="
echo "Demo complete. Open $BASE/ui to drive these interactively."
echo "=================================================="
