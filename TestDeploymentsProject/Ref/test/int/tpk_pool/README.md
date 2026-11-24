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

Venue A (`bench-ref.json`): `<root>/uut-ref/` is `lestarch-autobot/fprime` on branch
`tlm-packetizer-membership-pool-harness` (the unit under test plus commits under `TestDeploymentsProject/Ref` only).
It is both the image source and the harness checkout:

```sh
(cd uut-ref/TestDeploymentsProject/Ref && fprime-util generate -DFPRIME_TLM_PACKETIZER_OMIT_LIST=OFF && fprime-util build)
T=uut-ref/TestDeploymentsProject/Ref/test/int/tpk_pool
I=uut-ref/TestDeploymentsProject/build-artifacts/Linux/Ref/bin/Ref
fth manifest --uut uut-ref --harness uut-ref --plan $T/plan.json --menu $T/faults-menu.json --bench $T/bench-ref.json --tests $T --image Ref=$I -o manifest.json
```

The same plan and stimulus files are kept beside the Zephyr reference deployment
(`lestarch-autobot/fprime-zephyr-reference`, same branch name, `FprimeZephyrReference/ReferenceDeployment/test/int/tpk_pool`);
`diff -r` of the two directories shows only the venue files (`bench-*.json`, `venue/`).

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
`SOURCE.json`) and the manifest of those runs. `../../campaign/dry-runs.sh` makes the runs, replays each record
(`fth run --replay-record`), builds the set (`../../campaign/mk-vectors.py`) and applies the vector rule:

```sh
python3 $T/venue/check-vectors.py $T/../../vectors     # one line per vector; exit 1 on a mismatch or an empty set
```

`fth vectors` (v0.1.29) cannot judge a plan whose pass rules live in the tests; `check-vectors.py` applies its rule
(`fth.vectors.verdict_problems`, imported unmodified) to the replay verdicts.
The campaign scripts sit outside this directory (`../../campaign/`) because they name the planted fault of each
run, which the stimulus directory must not (`fth verify-plan` refuses it).

## Notes for the reader of a record

- **L0-1 and the image identity.** The test's check is `FrameworkVersion count >= 1`, as in the plan. That the
  image carries the manifest's describe strings is judged by the tooling, not by this test: `fth manifest`
  refuses an image without them and `fth audit` compares the dictionary versions with the manifest.
- **Two packets, one sample.** `TpkA` (60) and `TpkB` (61) both carry `cmdDisp.CommandsDispatched`. With both
  enabled (L0-2, L7-1) each change is downlinked twice with one time tag; the recorder writes the lines one after
  the other and the tool's export collapses adjacent identical lines, so the window holds one sample per change.
  The helper does not collapse equal samples: a second copy that is not adjacent (a held packet released later
  with its old time tag, as in the teardown windows of L2-1 and L2-2) stays a second row, and in a judged
  window it would be a step of 0 and FAIL. `test/kat` pins both.
- **State a planted fault leaves behind.** A test's teardown clears only the test's own override.
  `first_packet_also_held` leaves packet 60 held from the setup of L2-1 until the teardown of L2-2 (L2-2 holds
  packet 60 itself). `second_packet_also_held` leaves packet 61 held from the setup of L2-2 through L6-1 until
  the setup of L7-1. `final_packets_held` leaves both held at the end of the run. In plan order no other verdict
  changes (L6-1 judges no packet flow), so `ripples_to` is empty; a single test run after a planted run starts
  from a new image.
