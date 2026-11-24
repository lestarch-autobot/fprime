"""Compare the verdicts of a replay of the committed vector records with the ones kept in each vector.

usage: compare-verdicts.py <vectors dir> <dir of the replay run dirs>
One line per comparison (the run, its replay, its audit); exit 0 when all are equal."""
import json, sys
V, rep = sys.argv[1], sys.argv[2]; bad = 0
for n in range(1, 6):
    new = json.load(open(f'{rep}/dry-{n}/runner/verdicts.json'))['tests']
    for ref in ('runner/verdicts.json', 'replay-verdicts.json', 'audit/verdicts.json'):
        old = json.load(open(f'{V}/dry-{n}/{ref}')); old = old.get('tests', old)
        eq = new.keys() == old.keys() and all((new[k]['verdict'], sorted(new[k]['failed'])) == (old[k]['verdict'], sorted(old[k]['failed'])) for k in new)
        bad += not eq
        print(f'dry-{n} vs {ref}:', 'equal' if eq else 'DIFFER')
    print('   ', ' '.join(f"{k}={t['verdict']}({len(t['failed'])})" for k, t in new.items()))
sys.exit(bad)
