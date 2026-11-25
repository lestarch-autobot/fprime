# TPK-POOL system tests (Svc.TlmPacketizer membership pool)

External stimulus of the approved plan `plan.json` (rendered: `plan.md`). The unit under test is not
changed by anything here. Tooling: `fprime-testing-harness` v0.1.29 (`fth`), `fprime-gds`.

| file | what |
|---|---|
| `plan.json`, `plan.md` | the approved plan and its rendering (`fth render-plan plan.json -o plan.md`) |
| `planted_faults.py`, `faults-menu.json` | plan helper and planted faults; the menu is generated (`fth planted-faults-menu plan.json`) |
| `test_L*.py` | one stimulus file per plan test |
| `bench-ref.json`, `venue/` | venue A: `Ref` on a Linux host, GDS over TCP; scripts that start, stop and read back the image |
| `../../kat/` | known-answer tests of the plan helper (`pytest test/kat`) |

## Layout of a campaign root

All paths in `bench-*.json` are relative to the campaign root, the directory every command below is run from.

Venue A (`bench-ref.json`): `<root>/uut/` is `lestarch-autobot/fprime` on branch
`tlm-packetizer-membership-pool-harness` (the unit under test plus commits under `TestDeploymentsProject/Ref` only).
It is the image source and holds the tests: the manifest names it `uut`, `project` and `fprime` (one repository, one commit).
`fth rfr` accepts only paths under `uut/`, `harness/` and `rfr/` (its output), so the test tree must have this name. `<root>/harness/` is the
pinned `fprime-testing-harness` checkout that `fth` is installed from: the manifest names it `harness`, the shape the
run for record uses (plan `image_version.harness_in_image` is false: the tool is not in the image):

```sh
(cd uut/TestDeploymentsProject/Ref && fprime-util generate -DFPRIME_TLM_PACKETIZER_OMIT_LIST=OFF && fprime-util build)
T=uut/TestDeploymentsProject/Ref/test/int/tpk_pool
I=uut/TestDeploymentsProject/build-artifacts/Linux/Ref/bin/Ref
fth manifest --uut uut --harness harness --repo project=uut --repo fprime=uut --plan $T/plan.json --menu $T/faults-menu.json --bench $T/bench-ref.json --tests $T --image Ref=$I -o manifest.json
```

The same plan and stimulus files are kept beside the Zephyr reference deployment
(`lestarch-autobot/fprime-zephyr-reference`, same branch name, `FprimeZephyrReference/ReferenceDeployment/test/int/tpk_pool`);
`diff -r` of the two directories shows only the venue files (`bench-*.json`, `venue/`).

## Complete procedure (venue A, from a new machine)

Needs: a Linux x86_64 host, `git`, Python 3 with `venv`, a C++ compiler with its `gcov`, CMake. Nothing here uses
a bench other than the host itself. One directory, the campaign root, holds everything; every command is run
from it unless a `cd` is shown. About 40 minutes.

```sh
# The three repositories are private: git needs a credential for github.com/lestarch-autobot (a token with read
# access, e.g. `gh auth setup-git` or a credential helper), also for the clone of `fprime-testing-harness`.
D=tpk-campaign-a-$(date +%Y%m%d-%H%M); [ ! -e $D ] || { echo "$D exists: wait a minute, never reuse a campaign directory"; exit 1; }
mkdir $D && cd $D                                  # a new, dated directory; not the one of venue B
B=tlm-packetizer-membership-pool-harness
git clone --recurse-submodules -b $B https://github.com/lestarch-autobot/fprime uut          # image source and tests
git clone --recurse-submodules -b $B https://github.com/lestarch-autobot/fprime uut-cov      # same commit, coverage image
git clone --recurse-submodules -b tlm-packetizer-membership-pool https://github.com/lestarch-autobot/fprime unit   # unit under test, for its unit tests
git -C unit rev-parse HEAD                      # 14dc2e3077b1a5b108a067805abac48c1f765be9
git -C uut rev-parse HEAD; git -C uut-cov rev-parse HEAD      # both the harness head named in the hand-off; stop if not
python3 -m venv venv && . venv/bin/activate
pip install -r uut/requirements.txt                  # pins gcovr 8.2
git clone https://github.com/lestarch-autobot/fprime-testing-harness harness && git -C harness checkout --detach v0.1.29   # fth; commit fb09828
pip install -e "harness[test]"                           # fth runs from the checkout the manifest pins as `harness`
python -c "import fth.cli, os; assert os.path.realpath(fth.cli.__file__).startswith(os.path.realpath('harness')), fth.cli.__file__"
fth selftest | tail -1                                   # selftest ok
T=uut/TestDeploymentsProject/Ref/test/int/tpk_pool
```

1. Build the three trees (the image carries `git describe` of its tree: build only a clean checkout).

```sh
(cd uut/TestDeploymentsProject/Ref && fprime-util generate -DFPRIME_TLM_PACKETIZER_OMIT_LIST=OFF && fprime-util build -j4) > ref-build.log 2>&1; echo "ref build rc=$?"
(cd uut-cov/TestDeploymentsProject/Ref && fprime-util generate -DFPRIME_TLM_PACKETIZER_OMIT_LIST=OFF -DCMAKE_CXX_FLAGS=--coverage -DCMAKE_C_FLAGS=--coverage -DCMAKE_EXE_LINKER_FLAGS=--coverage && fprime-util build -j4) > cov-build.log 2>&1; echo "cov build rc=$?"
(cd unit && fprime-util generate --ut) > ut-generate.log 2>&1; echo "ut generate rc=$?"
grep -E "MEMBERSHIPS = |CHANNELS = |needs MAX" uut/TestDeploymentsProject/build-fprime-automatic-native/Ref/RefTlmPacketizerSizing.cpp
```

Expected: three `rc=0`, and from the generated sizing check of the unit under test (its `static_assert`
messages are compiled, not printed, when the build passes):

```text
static constexpr FwSizeType MEMBERSHIPS = 178;
static constexpr FwSizeType CHANNELS = 177;
              "Packet set RefPackets needs MAX_PACKETIZER_MEMBERSHIPS >= 178");
              "Packet set RefPackets needs MAX_PACKETIZER_CHANNELS >= 177 (177 listed in packets; 25 omitted not counted, FPRIME_TLM_PACKETIZER_OMIT_LIST is OFF)");
```

2. Bench file. `bench-ref.json` names the host in `serial` (`devin-box`); `start-uut.sh` prints `hostname` in the
   place of a debugger serial and the audit compares the two. On a host with another name the audit's step
   "bench file resolved (serial comparison)" fails: either run on a host of that name, or change `serial`,
   commit, and rebuild (the file is pinned by the manifest and the commit is in the image's version string).
   That makes a head nobody reviewed: state it in the run report with the diff, which must be that one line.
   The image listens on TCP port 50000 of 127.0.0.1; the port must be free.

3. Static checks, no bench.

```sh
fth verify-plan $T/plan.json --tests $T          # plan ok: ... 6 tests, 1 helpers, 4 planted faults, 1 waived requirements, 0 warnings
fth verify-stimulus $T/plan.json --tests $T      # 6 lines MATCH, 0 FAIL
pytest -q uut/TestDeploymentsProject/Ref/test/kat      # 30 passed
```

4. The five dry runs, audit, replay, vector set and vector rule: one script. It makes `manifest.json`, starts the
   image for each run (`venue/start-uut.sh`, read back with `venue/readback-uut.sh`) and stops it by its
   recorded pid. The planted fault of each run is named only in `../../campaign/orchestrate-vectors.sh`
   (dry-2 `L2-1=first_packet_also_held`, dry-3 `L2-2=second_packet_also_held`, dry-4 `L6-1=known_packet_requested`,
   dry-5 `L7-1=final_packets_held`, each as `fth run --unsealed --plant <test>=<fault>`).

```sh
sh uut/TestDeploymentsProject/Ref/test/campaign/dry-runs.sh 2>&1 | tee venue-a.out
grep -a -h MATCH runs/dry-?/record/raw/flash.log           # 5 lines "MATCH: pid <n> runs uut/..."
```

Expected in `venue-a.out` (`<V>` is the check name `cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2
increasing samples, each step == dispatches between them, a sample within 2 ticks after each dispatch`; record
ids differ from run to run):

```text
manifest <id> -> manifest.json ...
run dry-1: record <id> -> runner/verdicts.json (pytest exit 0)
dry-1 rc=0
  L0-1 PASS []          (and L0-2, L2-1, L2-2, L6-1, L7-1: PASS [])
  audit rc=0 FAILED-lines=0 DIVERGENCE=0
  replay equal
run dry-2: record <id> -> runner/verdicts.json (pytest exit 1)
dry-2 rc=0
  L2-1 FAIL ['<V>']     (the five others PASS [])
  audit rc=1 FAILED-lines=3 DIVERGENCE=0
  replay equal
dry-3: as dry-2 with L2-2 FAIL ['<V>']
dry-4: L6-1 FAIL ['event cmdDisp.OpCodeError count == 1', 'event tlmSend.PacketNotFound(packetId=999) count == 1', 'event tlmSend.PacketSent count == 0']
dry-5: as dry-2 with L7-1 FAIL ['<V>']
vectors: 5 lines "ok", one per vector
vector rule rc=0
```

   The audit of a planted run ends FAILED by the tool's design (`audit rc=1`, two failed steps: "no development
   plant in the record" and "replay sent exactly the recorded commands"); its runner-versus-replay table must
   still show no DIVERGENCE. Only the clean run's audit ends ok (`runs/dry-1/auditor/`, table
   `diff-runner-replay.md`).

5. Stage-3 rehearsal: a REHEARSAL, not a record. The run-for-record commands once, with a throwaway seed that is
   never printed (`fth draw` deletes the file it reads, so a copy is kept for `fth unseal`), run id `reh-1`:

```sh
(umask 077; head -c 32 /dev/urandom | base64 > seed.txt; cp seed.txt seed-unseal.txt)
rm -f /tmp/fth-tpk-pool-*
fth draw --manifest manifest.json --run-dir runs/reh-1 --seed-file seed.txt > runs/reh-1.log 2>&1; echo "draw rc=$?"
fth run --run-dir runs/reh-1 --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T >> runs/reh-1.log 2>&1; echo "run rc=$?"
cp ref-uut/uut.out runs/reh-1.uut.out; $T/venue/stop-uut.sh; sleep 2
fth audit --run-dir runs/reh-1 --manifest manifest.json --plan $T/plan.json --tests $T --bench $T/bench-ref.json >> runs/reh-1.log 2>&1; echo "audit rc=$?"
fth unseal --run-dir runs/reh-1 --manifest manifest.json --seed-file seed-unseal.txt -o runs/reh-1/unseal.md >> runs/reh-1.log 2>&1; echo "unseal rc=$?"
rm -f seed.txt seed-unseal.txt; grep -c -i "traceback" runs/reh-1.log
```

   Expected: `draw rc=0`; `run rc=0` (the sealed run does not show its verdicts; `runs/reh-1.log` names the record);
   the log line `run reh-1: record <id> ... (pytest exit 1)` is normal when the draw planted a fault (a planted test
   fails by design) and is not a tooling error; `fth run` itself returns 0;
   `audit rc=0` and `unseal rc=0`; `0` tracebacks: no tooling error. The verdicts are not evidence of anything.

6. Coverage and `fth dev-campaign` (uses `runs/dry-1` and `manifest.json` of step 4; give the `gcov` of the
   compiler that built the trees if it is not the first `gcov` on the path):

```sh
sh uut/TestDeploymentsProject/Ref/test/campaign/coverage.sh
```

Expected: `cov-1 rc=0`, some hundreds of `.gcda` files (315 here), last lines
`dev-evidence -> dev-evidence (CLEAN)` and `dev-campaign rc=0`; in `dev-evidence/README.md` unit-test lines
95.2 % (plan threshold 95) and system lines 56.7 % (threshold 50) with gcc 15.3.

`coverage.sh` and the `mkdir` of `dry-runs.sh` were written for this procedure from the commands used for the
committed evidence; the script as a whole had not been run when it was committed.
`coverage.sh` writes `unit/Svc/TlmPacketizer/coverage/print-summary.txt` from the three gcovr totals in
`ut-check.log` (gcovr prints them to the log only) and stops if they are missing; nothing is typed in by hand.

## Run for record (`fth rfr`)

From a new dated campaign root that holds `uut/` (this branch) and `harness/` (v0.1.29), both clean at the pins of
the package's manifest, the venv with `fth` installed from `harness/`, `rfr-input.zip` and `rfr-input.zip.sha256`.
No build is needed: image and dictionary come from the package. The seed file is chmod 600, at the root (not
inside a checkout: an untracked file there makes the tree dirty) and is deleted when read. Port 50000 must be
free and the host must have the name in `bench-ref.json` `serial`. `rfr/` must not exist from another package.

```sh
fth rfr rfr-input.zip --dry-run                    # rehearsal OK, no seed, nothing started
rm -f /tmp/fth-tpk-pool-*
fth rfr rfr-input.zip --seed-file seed.txt; echo "rfr rc=$?"
uut/TestDeploymentsProject/Ref/test/int/tpk_pool/venue/stop-uut.sh
```

Stop at the first refusal and report it verbatim; nothing is retried or assembled by hand.

## Static checks (no bench)

```sh
fth verify-plan $T/plan.json --tests $T
fth verify-stimulus $T/plan.json --tests $T
pytest $T/../../kat
```

## Dry run (no bench, nothing sent)

```sh
fth run --dry-run --run-dir runs/dry-0 --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T
```

## Whole plan on the bench (development, unsealed)

```sh
fth run --unsealed --run-dir runs/dry-1 --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T
$T/venue/stop-uut.sh
```

`fth run` starts the image (`start-uut.sh`, read back with `readback-uut.sh`, MATCH/MISMATCH in
`record/raw/flash.log`), the GDS and one recorder, runs the six tests in plan order and stops the GDS.
One planted fault alone: add `--plant L2-1=first_packet_also_held` (development only).

## One test

Append the pytest selection after `--`:

```sh
fth run --unsealed --run-dir runs/one-1 --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T -- -k test_L2_1
```

## Replay of a record

```sh
fth run --replay-record runs/dry-1/record --run-dir runs/dry-1-replay --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T
fth audit --run-dir runs/dry-1 --manifest manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T --unsealed
```

## Mutation vectors

`../../vectors/` holds the vector set of this venue: one clean record and one record per planted fault applied
alone, as the dry runs made them (`vector.json`, `record/` with `record.sha256`, `sealed/faults.json`, `runner/`,
`replay-verdicts.json`, `audit/`, the logs), the plan copy they were judged with (`plans/tpk-pool/`, hashes in
`SOURCE.json`) and two manifests: `manifest-as-run.json`, the one the runs were made with, and `manifest.json`,
made at this branch for the replay (stage-3 shape; plan and stimulus hashes of this branch; its image entry is a build at the first
amendment-9 commit of this branch, which is not the image of the runs).
`record.sha256` is the tool's list for the whole record and also names the `raw/` files, which are not
committed (they are in the evidence bundle); `files.sha256` lists the files that are committed.
`rejudge/` holds the verdicts of a replay of each record with the plan files of this branch (amendment 9, which changes no test, export or pass rule),
after `fth export --plan` of the record's raw logs with the plan's 60 s reboot limit (the export is byte-equal
to the committed `channels.jsonl` and `events.jsonl`): verdict and failed checks equal to `runner/verdicts.json` in every vector. `../../campaign/dry-runs.sh` makes the runs, replays each record
(`fth run --replay-record`), builds the set (`../../campaign/mk-vectors.py`) and applies the vector rule:

```sh
python3 $T/../../campaign/check-vectors.py $T/../../vectors     # one line per vector; exit 1 on a mismatch or an empty set
```

`fth vectors` (v0.1.29) cannot judge a plan whose pass rules live in the tests; `check-vectors.py` applies its rule
(`fth.vectors.verdict_problems`, imported unmodified) to the replay verdicts.
To repeat the gate on the committed records (no bench; from a campaign root, tools installed as above):

```sh
V=$T/../../vectors
for n in 1 2 3 4 5; do fth run --replay-record $V/dry-$n/record --run-dir replay/dry-$n --manifest $V/manifest.json --plan $T/plan.json --bench $T/bench-ref.json --tests $T; done
python3 $T/../../campaign/compare-verdicts.py $V replay      # 15 lines "equal": against the run, its replay and its audit; exit 0
(cd $V/dry-1 && sha256sum -c --quiet files.sha256)               # the committed files of a vector
```
The campaign scripts sit outside this directory (`../../campaign/`) because they name the planted fault of each
run, which the stimulus directory must not (`fth verify-plan` refuses it).

## Notes for the reader of a record

- **L0-1 and the image identity.** The test's check is `FrameworkVersion count >= 1`, as in the plan. That the
  image carries the manifest's describe strings is judged by the tooling, not by this test: `fth manifest`
  refuses an image without them and `fth audit` compares the dictionary versions with the manifest.
- **Two packets, one sample.** `TpkA` (60) and `TpkB` (61) both carry `cmdDisp.CommandsDispatched`. With both
  enabled (L0-2, L7-1) each change is downlinked twice with one time tag. The tool's export collapses the two
  copies when their recorder lines are adjacent. They need not be: on `Ref` the channel is written on the
  rate-group thread while the packetizer sends on its own, so one copy can leave a tick after the other, and a
  held packet released later carries its old time tag (teardown windows of L2-1 and L2-2). The helper therefore
  drops a sample equal in time and value to the one before it (plan amendment 7) and nothing else: an equal
  value with a later time is a step of 0 and FAIL. `test/kat` pins both.
  The filter is exact only while `TpkA` and `TpkB` list this one channel and no other, so that a packet's time
  is the time of this channel's write: `TestDeploymentsProject/Ref/Top/RefPackets.fppi:303-309`.
- **State a planted fault leaves behind.** A test's teardown clears only the test's own override.
  `first_packet_also_held` leaves packet 60 held from the setup of L2-1 until the teardown of L2-2 (L2-2 holds
  packet 60 itself). `second_packet_also_held` leaves packet 61 held from the setup of L2-2 through L6-1 until
  the setup of L7-1. `final_packets_held` leaves both held at the end of the run. In plan order no other verdict
  changes (L6-1 judges no packet flow), so `ripples_to` is empty; a single test run after a planted run starts
  from a new image.
