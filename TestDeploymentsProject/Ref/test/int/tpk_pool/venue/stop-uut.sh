#!/bin/sh
# Stop the UUT process start-uut.sh recorded (by that pid only, never by name).
set -eu
state="ref-uut"
[ -f "$state/pid" ] || exit 0
pid="$(cat "$state/pid")"
if kill -0 "$pid" 2>/dev/null; then
    kill "$pid"
    i=0; while kill -0 "$pid" 2>/dev/null && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
    kill -0 "$pid" 2>/dev/null && kill -9 "$pid"
fi
rm -f "$state/pid"
