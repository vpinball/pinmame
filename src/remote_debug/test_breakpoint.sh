#!/bin/bash

PORT=${PORT:-8926}
ROM=${ROM:-taf_l7}
BINARY=${BINARY:-../../xpinmamed.x11}
ROMPATH=${ROMPATH:-$HOME/.pinmame}
ADDR="0x8CBE"

ROMPATH_ARGS=()
[ -d "$ROMPATH" ] && ROMPATH_ARGS=(-rompath "$ROMPATH")

echo "--------------------------------------------------"
echo "PinMAME Breakpoint Test Script"
echo "ROM: $ROM, Address: $ADDR, Port: $PORT"
echo "--------------------------------------------------"

# Kill any existing process
killall -9 xpinmamed.x11 2>/dev/null

echo "1. Starting PinMAME in background..."
$BINARY -headless -startpaused -httpport $PORT -nosound "${ROMPATH_ARGS[@]}" $ROM > test_log.txt 2>&1 &
PID=$!

# Wait for init
sleep 5

if ! ps -p $PID > /dev/null; then
    echo "ERROR: PinMAME failed to start. Log content:"
    cat test_log.txt
    exit 1
fi

echo "2. Setting breakpoint at $ADDR..."
curl -s "http://localhost:$PORT/api/debugger/command?cmd=bp%20$ADDR"
echo ""

echo "3. Resuming emulation..."
curl -s "http://localhost:$PORT/api/debugger/control?cmd=resume"
echo ""

echo "4. Waiting for breakpoint hit..."
MAX_WAIT=20
COUNT=0
HIT=0

while [ $COUNT -lt $MAX_WAIT ]; do
    STATE=$(curl -s "http://localhost:$PORT/api/info")
    MSGS=$(curl -s "http://localhost:$PORT/api/debugger/messages")
    
    PAUSED=$(echo $STATE | grep -o '"paused":1')
    HALT_MSG=$(echo $MSGS | grep -o 'Halt:')
    
    if [ ! -z "$PAUSED" ] || [ ! -z "$HALT_MSG" ]; then
        echo "SUCCESS: Breakpoint triggered!"
        echo "Final State: $STATE"
        echo "Messages: $MSGS"
        HIT=1
        break
    fi
    
    echo "Still running... ($COUNT)"
    sleep 1
    ((COUNT++))
done

if [ $HIT -eq 0 ]; then
    echo "FAILURE: Breakpoint was never hit."
    kill -9 $PID 2>/dev/null
    exit 1
fi

kill -9 $PID 2>/dev/null
echo "Test complete."
