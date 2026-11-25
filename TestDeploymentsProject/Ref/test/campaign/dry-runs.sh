#!/bin/sh
# Venue A full cycle, run from the campaign root: manifest, clean run, 4 planted runs, audit, replay, vector set, vector rule.
T=uut-ref/TestDeploymentsProject/Ref/test/int/tpk_pool
I=uut-ref/TestDeploymentsProject/build-artifacts/Linux/Ref/bin/Ref
mkdir -p runs; rm -f /tmp/fth-tpk-pool-*
fth manifest --uut uut-ref --harness harness --repo project=uut-ref --plan $T/plan.json --menu $T/faults-menu.json --bench $T/bench-ref.json --tests $T --image Ref=$I -o manifest.json 2>&1 | tail -1 | cut -c1-200
fth run --unsealed --run-dir runs/dry-1 --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T > runs/dry-1.log 2>&1
echo "dry-1 rc=$?" >> runs/dry-1.log; cp ref-uut/uut.out runs/dry-1.uut.out; $T/venue/stop-uut.sh; sleep 2
$T/../../campaign/orchestrate-vectors.sh
for n in 1 2 3 4 5; do
  tail -2 runs/dry-$n.log | cut -c1-200
  python3 -c "
import json;v=json.load(open('runs/dry-$n/runner/verdicts.json'))
for k,t in v['tests'].items(): print(' ',k,t['verdict'],t['failed'])"
  fth audit --unsealed --run-dir runs/dry-$n --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T > runs/dry-$n.audit.log 2>&1
  echo "  audit rc=$? FAILED-lines=$(grep -c FAILED runs/dry-$n.audit.log) DIVERGENCE=$(grep -ci divergence runs/dry-$n.audit.log)"
  fth run --replay-record runs/dry-$n/record --run-dir runs/dry-$n-replay --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T > runs/dry-$n.replay.log 2>&1
  python3 - <<P
import json
a=json.load(open('runs/dry-$n/runner/verdicts.json'))['tests'];b=json.load(open('runs/dry-$n-replay/runner/verdicts.json'))['tests']
print('  replay', 'equal' if a.keys()==b.keys() and all((a[k]['verdict'],a[k]['failed'])==(b[k]['verdict'],b[k]['failed']) for k in a) else 'DIFFER')
P
done
rm -rf vectors && python3 $T/../../campaign/mk-vectors.py runs $T vectors "venue A (Ref, Linux host), fth run --unsealed, fprime harness $(git -C uut-ref rev-parse --short=9 HEAD)" && cp manifest.json vectors/
python3 $T/../../campaign/check-vectors.py vectors; echo "vector rule rc=$?"
