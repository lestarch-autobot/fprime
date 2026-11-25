#!/bin/sh
# Venue A coverage and `fth dev-campaign`, run from the campaign root after dry-runs.sh (it needs runs/dry-1 and
# manifest.json). Needs beside the root's uut-ref/ and harness/ (the pinned fth checkout): uut-ref-cov/ (the same commit, built with --coverage) and
# uut/ (the unit under test alone, for its unit tests). See the README for how the three are made.
# usage: coverage.sh [gcov]   (the gcov of the compiler that built uut-ref-cov; default `gcov`)
set -eu
G=${1:-gcov}
root=$PWD
T=uut-ref/TestDeploymentsProject/Ref/test/int/tpk_pool
B=$root/uut-ref-cov/TestDeploymentsProject/build-fprime-automatic-native
[ "$(git -C uut-ref rev-parse HEAD)" = "$(git -C uut-ref-cov rev-parse HEAD)" ] || { echo "uut-ref-cov is not at the commit of uut-ref"; exit 1; }
# unit-test coverage of the component (default-config project)
(cd uut/Svc/TlmPacketizer && fprime-util check --coverage > "$root/ut-check.log" 2>&1) || { echo "unit tests failed, see ut-check.log"; exit 1; }
# gcovr prints its totals ("lines: X% (a out of b)") only to the log; `fth dev-campaign` reads them from a file
grep -E '^(lines|functions|branches): [0-9.]+% \([0-9]+ out of [0-9]+\)$' ut-check.log > uut/Svc/TlmPacketizer/coverage/print-summary.txt || true
[ "$(wc -l < uut/Svc/TlmPacketizer/coverage/print-summary.txt)" -eq 3 ] || { echo "ut-check.log does not hold the three gcovr totals"; exit 1; }
# the plan once on the instrumented image, in its own root (the bench file's paths start at uut-ref/)
mkdir -p cov/runs && ln -sfn ../uut-ref-cov cov/uut-ref && ln -sfn ../harness cov/harness   # the manifest names both
find "$B" -name '*.gcda' -delete
(cd cov && rm -rf runs/cov-1 gcda && rm -f /tmp/fth-tpk-pool-* \
  && fth manifest --uut uut-ref --harness harness --repo project=uut-ref --plan $T/plan.json --menu $T/faults-menu.json --bench $T/bench-ref.json --tests $T \
       --image Ref=uut-ref/TestDeploymentsProject/build-artifacts/Linux/Ref/bin/Ref -o manifest.json | tail -1 | cut -c1-200 \
  && fth run --unsealed --run-dir runs/cov-1 --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T > runs/cov-1.log 2>&1; \
  echo "cov-1 rc=$?"; tail -1 runs/cov-1.log | cut -c1-200; $T/venue/stop-uut.sh)   # the image writes its .gcda files when it is stopped
sleep 2
(cd "$B" && find . -name '*.gcda' | while read -r f; do mkdir -p "$root/cov/gcda/$(dirname "$f")"; cp "$f" "$root/cov/gcda/$f"; done)
echo "gcda files: $(find cov/gcda -name '*.gcda' | wc -l)"
rm -rf dev-evidence
fth dev-campaign --out dev-evidence --plan $T/plan.json --tests $T --bench $T/bench-ref.json \
  --ut-coverage uut/Svc/TlmPacketizer/coverage \
  --coverage-run cov/runs/cov-1 --gcda-dir cov/gcda --coverage-manifest cov/manifest.json \
  --gcno-root "$B" --project-root uut-ref-cov --filter Svc/TlmPacketizer/ --gcov "$G" \
  --pure-run runs/dry-1 --pure-manifest manifest.json > dev-campaign.log 2>&1 && rc=0 || rc=$?
tail -1 dev-campaign.log; echo "dev-campaign rc=$rc"
exit $rc
