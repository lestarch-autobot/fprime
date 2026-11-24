"""Build the vector set from finished dry runs: usage mk-vectors.py <runs> <tests-dir> <out> <note>.

Each vector holds the exported record, the hand run's `runner/`, the verdicts of `fth run --replay-record` over
the record (replay-verdicts.json, from <runs>/<run>-replay/), the audit's files and logs, and the guard output
where the venue has one. The planted fault of a vector is read from the run's own runner/verdicts.json (`dev_plant`, written by
`fth run --plant`); nothing here names a run or a fault."""
import hashlib, json, shutil, sys
from pathlib import Path

runs, tests, out, note = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]), sys.argv[4]
pdir = out / "plans" / "tpk-pool"; pdir.mkdir(parents=True)
files = ["plan.json", "faults-menu.json", "planted_faults.py"] + sorted(p.name for p in tests.glob("test_L*.py"))
for f in files:
    shutil.copy(tests / f, pdir / f)
(pdir / "SOURCE.json").write_text(json.dumps({"note": note, "files": {f: hashlib.sha256((pdir / f).read_bytes()).hexdigest() for f in files}}, indent=2) + "\n")
for run in sorted(p for p in runs.iterdir() if (p / "record").is_dir() and (runs / (p.name + "-replay")).is_dir()):
    d = out / run.name; (d / "record").mkdir(parents=True)
    for f in (run / "record").iterdir():
        if f.is_file():
            shutil.copy(f, d / "record" / f.name)
    shutil.copy(runs / (run.name + "-replay") / "runner" / "verdicts.json", d / "replay-verdicts.json")
    shutil.copy(run / "record.sha256", d / "record.sha256")
    shutil.copytree(run / "runner", d / "runner")
    (d / "audit").mkdir()
    for f in sorted((run / "auditor").iterdir()):
        if f.is_file():
            shutil.copy(f, d / "audit" / f.name)
    for ext in ("audit.log", "replay.log", "guard.out"):
        if (runs / f"{run.name}.{ext}").is_file():
            shutil.copy(runs / f"{run.name}.{ext}", d / ext)
    plant = json.loads((run / "runner" / "verdicts.json").read_text()).get("dev_plant") or {}
    if plant:
        (d / "sealed").mkdir(); (d / "sealed" / "faults.json").write_text(json.dumps({"faults": plant}, indent=2) + "\n")
    (d / "vector.json").write_text(json.dumps({"kind": "single-knob" if plant else "clean", "note": f"{note}, run {run.name}" + (f", planted {plant}" if plant else ""), "plan": "plans/tpk-pool/plan.json"}, indent=2) + "\n")
    print(run.name, "single-knob" if plant else "clean", plant)
