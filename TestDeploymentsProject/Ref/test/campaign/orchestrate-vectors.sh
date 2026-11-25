#!/bin/sh
# Venue A mutation vectors: the planted fault -> run id mapping lives only here.
T=uut/TestDeploymentsProject/Ref/test/int/tpk_pool
n=2
for p in L2-1=first_packet_also_held L2-2=second_packet_also_held L6-1=known_packet_requested L7-1=final_packets_held; do
  rm -f /tmp/fth-tpk-pool-*
  fth run --unsealed --plant $p --run-dir runs/dry-$n --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T > runs/dry-$n.log 2>&1
  echo "dry-$n rc=$?" >> runs/dry-$n.log
  cp ref-uut/uut.out runs/dry-$n.uut.out; $T/venue/stop-uut.sh
  sleep 2
  n=$((n+1))
done
