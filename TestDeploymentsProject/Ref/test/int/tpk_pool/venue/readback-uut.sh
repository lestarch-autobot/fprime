#!/bin/sh
# bench.json `dump` of the native venue: copy the executable the recorded UUT process is running.
# usage: readback-uut.sh <image> <readback>
set -eu
pid="$(cat ref-uut/pid)"
cp "/proc/$pid/exe" "$2"
if cmp -s "$1" "$2"; then echo "MATCH: pid $pid runs $1"; else echo "MISMATCH: pid $pid does not run $1"; exit 1; fi
