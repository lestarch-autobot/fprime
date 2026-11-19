# SeqScheduler system tests

The 26 tests of [`plan.md`](plan.md) (rendered from `plan.json`), run on the `Ref` deployment
as a native Linux process with `fprime-gds`. They are written for the `fth` test tool
(`fprime-testing-harness`): each `test_<ID>` function sends the plan's procedure between two
record markers and declares the plan's named checks over the recorded window.

| File | Content |
|---|---|
| `plan.json`, `plan.md` | The approved plan and its rendering (`fth render-plan`) |
| `test_<ID>.py` | One file and function per plan test; stimulus and named checks (`fth verify-stimulus` compares each with the plan's procedure) |
| `seqsched_common.py` | Names, helpers and check functions shared by the tests |
| `../kat/test_helpers_kat.py` | Known-answer tests of the helper checks, one holding and one failing case each (`pytest Svc/SeqScheduler/test/kat`) |
| `planted_faults.py` | The seven planted faults and the completeness helpers |
| `faults-menu.json` | The fault menu (`fth planted-faults-menu plan.json`) |
| `bench.json` | The bench: how `Ref`, the GDS and the recorder are started and commanded |
| `files/` | Sequences and the parameter file the plan names |
| `../bench/seqsched_bench.py` | Bench actions: start and stop `Ref`, write and read table files, decode data products |

Test code reads no file: tables, data products and the start time of the `Ref` process reach
the checks as bench evidence in the record.

## Running

Build `Ref` (`TestDeploymentsProject/Ref`), then from the campaign directory:

```bash
I=uut/Svc/SeqScheduler/test/int
fth verify-plan $I/plan.json --tests $I --bench $I/bench.json
fth manifest --uut uut --harness uut --plan $I/plan.json --menu $I/faults-menu.json \
    --bench $I/bench.json --tests $I \
    --image Ref=uut/TestDeploymentsProject/build-artifacts/Linux/Ref/bin/Ref -o manifest.json
fth run --run-dir <new directory> --manifest manifest.json --plan $I/plan.json \
    --bench $I/bench.json --tests $I --unsealed
```

A full run takes about 22 minutes. Add `--plant <TEST>=<FAULT>` for a development planted-fault
run and `--replay-record <run>/record` to judge a recorded run again without the bench. Each
live run needs a new run directory.

## Limits

- The bench is a workstation: time bounds in the plan allow for the 1 Hz schedule tick and
  for 1 to 3.5 s of command latency through `fprime-cli`.
- `Ref` stamps `TB_WORKSTATION_TIME`; the test tool must keep each row's time base
  (`fprime-testing-harness` later than v0.1.2).
- The record has no ground receive time per row; L0-2 judges FSW time in the override span
  against the records before and after it.
