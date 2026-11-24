"""Mutation-vector gate for a plan whose pass rules live in the tests.

`fth vectors` (v0.1.29) judges with the engine, which refuses such a plan. This applies the same
rule, fth.vectors.verdict_problems, to the verdicts `fth run --replay-record` gave over each
vector's record (replay-verdicts.json in the vector)."""
import json, sys
from pathlib import Path
from fth.plan import load_plan
from fth.vectors import verdict_problems

root = Path(sys.argv[1]); ok = True
vdirs = sorted(p for p in root.iterdir() if (p / "record").is_dir())
if not vdirs:
    sys.exit(f"no vector directory under {root}")
for vdir in vdirs:
    plan = load_plan(root / json.loads((vdir / "vector.json").read_text())["plan"])
    bad = verdict_problems(plan, vdir, json.loads((vdir / "replay-verdicts.json").read_text())["tests"])
    ok &= not bad
    print(f"{'ok  ' if not bad else 'FAIL'} {vdir.name}  ({json.loads((vdir / 'vector.json').read_text())['kind']})")
    for b in bad:
        print("     " + b)
sys.exit(0 if ok else 1)
