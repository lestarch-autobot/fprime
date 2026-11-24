# Auditor report — run `dry-1`

Record `51e04da3a0bc18ad501610214e91cc753a163ea83231fa3811b6c091b32047c3` · checker `fth.replay` · 6 PASS / 0 FAIL / 0 UNVERIFIED

| test | title | verdict | failed checks |
|---|---|---|---|
| L0-1 | Image identity | PASS |  |
| L0-2 | Packets flow on change | PASS |  |
| L2-1 | Channel value reaches its first packet | PASS |  |
| L2-2 | Channel value reaches its second packet | PASS |  |
| L6-1 | Unknown packet is refused | PASS |  |
| L7-1 | Final state | PASS |  |

## Audit steps

- OK: bench file resolved (serial comparison)
- OK: manifest repo check — development run: repos + file hashes verified against this manifest at run start by fth run on the bench host (a failed check aborts the run); not re-verified on this host (development-run condition: runs consumed from the bench host; pass --repo-root to re-verify)
- OK: record hashes verified
- OK: run-start markers (unsealed development run: manifest marker only)
- OK: flash provenance (serial, image sha, read-back sha)
- OK: re-export from raw equals record
- OK: record integrity (one START..END per window, no command outside a window, monotonic time, export counts, junit hashed, no duplicate test)
- OK: replayed tests stay blind to the draw (no faults/sealed/run-dir/reflection identifiers in any *.py)
- OK: replay of the test functions over the record (/home/ubuntu/work/uut-ref/TestDeploymentsProject/Ref/test/int/tpk_pool, sha256 773e47591831…) → auditor/junit.xml, auditor/verdicts.json
- OK: no development plant (--fth-plant) in the record — a planted development run is never RfR
- OK: replay sent exactly the recorded commands (per test, in order; nothing left over at session end)
- OK: replay waits that timed out where the live stimulus did (record-derived; informational): none
- OK: runner junit (record/junit.xml, hashed) == replay, per test (verdict + failed + declared checks; auditor/diff-runner-replay.md)
- OK: runner/verdicts.json is what record/junit.xml yields
- OK: quick-look pages rendered (auditor/ql/)
- OK: inputs/ packaged (self-contained: manifest, plan, menu, bench, checker(s), toolchain, image(s), dictionary) — 5 files


Replayed test sources: pinned by the manifest (`stimulus`): uut@b93ae6f19c04 `uut-ref/TestDeploymentsProject/Ref/test/int/tpk_pool` — 8 file(s), each sha256-checked against the pin
## Runner (live, from record/junit.xml) vs replay (auditor)

| test | runner (live) | replay (auditor) | agree |
|---|---|---|---|
| L0-1 | PASS | PASS | yes |
| L0-2 | PASS | PASS | yes |
| L2-1 | PASS | PASS | yes |
| L2-2 | PASS | PASS | yes |
| L6-1 | PASS | PASS | yes |
| L7-1 | PASS | PASS | yes |
