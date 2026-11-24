#!/bin/sh
# bench.json `flash` of the native venue: (re)start the image as the UUT process.
# usage: start-uut.sh <image>   (run from the campaign root; state lives in ./ref-uut/)
set -eu
image="${1:?usage: start-uut.sh <image>}"
state="ref-uut"
"$(dirname "$0")/stop-uut.sh"
rm -rf "$state/cwd" && mkdir -p "$state/cwd"   # the image's working directory (parameter file, logs): empty at every start
abs="$(cd "$(dirname "$image")" && pwd)/$(basename "$image")"
( cd "$state/cwd" && exec setsid "$abs" -a 127.0.0.1 -p "${REF_UUT_PORT:-50000}" >../uut.out 2>&1 </dev/null ) &
pid=$!
echo "$pid" > "$state/pid"
sleep 1
kill -0 "$pid" 2>/dev/null || { echo "start-uut.sh: image exited at start, see $state/uut.out" >&2; exit 1; }
# A native venue has no debugger: the host identity stands in for the debugger serial.
echo "Serial# = $(hostname)"
echo "started pid $pid: $abs"
