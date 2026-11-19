"""Stimulus and named checks of the Svc.SeqScheduler system test plan (plan.json beside this file).

Each test sends the plan's procedure inside its windows and declares the plan's named checks.
The verdict is computed from the record at window close; waits are pacing only.
"""
import json
import re

import pytest

S = "Ref.seqSched"
B = "Ref.seqSchedB"
Q = "Ref.schedSeq"
QB = "Ref.schedSeqB"
T = "Ref.timeBaseSetter"
MARK = "CdhCore.cmdDisp.CMD_NO_OP_STRING"
SAMPLE = "seqSched.NextTimeDue"  # written every tick: one row per SeqSched packet
SAMPLE_B = "seqSchedB.NextTimeDue"


TB = {"TB_NONE": 0, "TB_PROC_TIME": 1, "TB_WORKSTATION_TIME": 2, "TB_SC_TIME": 3}
PLAN_PARAMS = {
    "seqSched.TABLE_FILE_PRM_SET": "seq_sched.tbl",
    "seqSchedB.TABLE_FILE_PRM_SET": "seq_sched_b.tbl",
    "seqSched.LATE_TOLERANCE_SEC_PRM_SET": 1,
    "seqSchedB.LATE_TOLERANCE_SEC_PRM_SET": 1,
}
IDLE = "Ref.schedSeq and Ref.schedSeqB idle at the opening marker"
PARAMS = "TABLE_FILE and LATE_TOLERANCE_SEC of Ref.seqSched and Ref.seqSchedB hold their plan values at the opening marker"
FRESH = "each 'FSW time + N s' is taken from an event received <= 1 s before the command is sent"
FRESH4 = "each 'FSW time + N s' is taken from an event received <= 4 s before the command is sent"


def reqs(numbers):
    """Requirement ids of the component from their numbers."""
    return ["REQ-SeqScheduler-" + n for n in numbers.split()]


def time_value(base, seconds):
    """A Fw.TimeValue command argument, in the JSON text a command takes on this bench."""
    return json.dumps({"timeBase": base, "timeContext": 0, "seconds": int(seconds), "useconds": 0})


def fsw_now(bench):
    """Seconds of the newest recorded event (0 in a dry run, where nothing is recorded)."""
    rows = bench.action("fsw_now")
    if bench.dry_run and bench.replay_record is None:
        return 0  # a dry run records nothing; a replay is also a dry run of the bench, but it has the readings
    if not rows or not rows[-1]:
        raise RuntimeError("bench: no FSW time reading (fsw_now printed no event)")
    return int(rows[-1]["sec"])


def fresh_times(limit=1.0):
    """The FSW times read in this window are at most `limit` seconds old.

    A reading is of this window when the event it names lies at or after the START marker and before the END
    marker. Evidence is kept per test, not per window: assigning the rows by event time makes a live run and a
    replay judge the same rows. Every window that reads an FSW time states this rule, so each reading is judged
    once. A window with no reading of its own fails; `fsw_now` raises on a reading that failed.
    """
    def judge(h):
        rows = h.evidence("fsw_now")
        first, last = h.window.start_marker.fsw, h.window.end_marker.fsw
        mine = [r for r in rows if r and first <= r["sec"] + r["usec"] / 1e6 < last]
        ages = [r["age_s"] for r in mine]
        return (len(mine) > 0 and max(ages) <= limit,
                f"ages {ages} of {len(rows)} FSW time readings in the test")
    return judge


def this_start(h):
    """Rows of the record from the last image start before the window opens up to its opening marker."""
    opening = h.window.start_marker
    events = [e for e in h.record.events if e.epoch == opening.epoch and e.t <= opening.t]
    commands = [c for c in h.record.commands]
    return events, commands


def sequencers_idle(h):
    """Every sequence either sequencer loaded since the image started has ended before the window opens."""
    events, _ = this_start(h)
    busy = {}
    for e in events:
        for q in ("schedSeq", "schedSeqB"):
            if e.short == q + ".CS_SequenceLoaded":
                busy[q] = True
            elif e.short in (q + ".CS_SequenceComplete", q + ".CS_SequenceCanceled", q + ".CS_CommandError"):
                busy[q] = False
    return (not any(busy.values()), f"running at the opening marker: {sorted(q for q, b in busy.items() if b)}")


def plan_params(h):
    """The last parameter set command of each plan parameter since the image started carries the plan value."""
    opening = h.window.start_marker
    start = min((e.fsw for e in h.record.events if e.epoch == opening.epoch), default=0.0)
    last = {}
    for c in h.record.commands:
        if c.short in PLAN_PARAMS and c.idx < h.window.cmds[0].idx:
            last[c.short] = c
    wrong = [f"{k}={c.raw}" for k, c in last.items() if str(PLAN_PARAMS[k]) not in c.raw]
    return (not wrong, f"start at {start}; off plan: {wrong}")


def standard_requires(require, fresh=False):
    require(IDLE, sequencers_idle)
    require(PARAMS, plan_params)
    if fresh == 4:
        # A window opened by the END marker of the one before: no event precedes its reading by under 1 s
        require(FRESH4, fresh_times(4.0))
    elif fresh:
        require(FRESH, fresh_times())


def run_time(h, row, window=None):
    w = h.windows[window] if window else h.window
    rows = [e for e in w.events("seqSched.SeqRun") if e.args.get("row") == row]
    return rows[0].fsw if rows else None


def run_not_before_row_time(row):
    """Row run at or after the time its (first) SET_ROW command of the window carries; RowSet reports that time."""
    def judge(h):
        run = run_time(h, row)
        sent = [c for c in h.window.cmds if c.short == "seqSched.SET_ROW" and c.args[:1] == [row]]
        sets = [e for e in h.window.events("seqSched.RowSet") if e.args.get("row") == row]
        found = re.search(r"'seconds': (\d+)", sets[0].text) if sets else None
        if run is None or found is None or not sent or len(sent[0].args) < 3:
            return (False, f"run {run}, SET_ROW commands {len(sent)}, RowSet {sets[0].text if sets else None}")
        seconds = sent[0].args[2]
        return (run >= seconds and int(found.group(1)) == seconds,
                f"run {run:.3f}, commanded row time {seconds}, RowSet {found.group(1)}")
    return judge


def run_not_before_delay(row):
    """Row run no earlier than the delay of its last RowSet in the window after the bench-recorded start of the
    Ref process (the first tick, from which the delay counts, is not before that start)."""
    def judge(h):
        run, start = run_time(h, row), [r for r in h.evidence("ref_start") if r]
        sets = [e for e in h.window.events("seqSched.RowSet") if e.args.get("row") == row]
        found = re.search(r"'seconds': (\d+)", sets[-1].text) if sets else None
        if run is None or found is None or not start:
            return (False, f"run {run}, RowSet {sets[-1].text if sets else None}, {len(start)} recorded starts")
        early = run - start[0]["start"] - int(found.group(1))
        return (early >= 0, f"{early:.3f} s after the start plus {found.group(1)} s")
    return judge


def command_time(h, name, nth=0, window=None):
    """FSW time at which the n-th sending of a command in the window was dispatched."""
    w = h.windows[window] if window else h.window
    rows = [c for c in w.cmds if c.short == name]
    if len(rows) <= nth:
        return None
    opcode = rows[nth].opcode
    sent = [e for e in w.events("cmdDisp.OpCodeDispatched") if f"0x{opcode:x}" in e.text.lower()]
    return sent[nth].fsw if len(sent) > nth else None


def run_after(row, name, low, high, nth=0):
    """Row run between low and high seconds after the n-th sending of a command (None: no bound)."""
    def judge(h):
        run, sent = run_time(h, row), command_time(h, name, nth)
        if run is None or sent is None:
            return (False, f"run {run}, command {sent}")
        delta = run - sent
        return ((low is None or delta >= low) and (high is None or delta <= high), f"{delta:.3f} s")
    return judge


def bracket(bench, n=2):
    """The plan's "bracket (n SeqSched samples)"."""
    bench.wait_samples(SAMPLE, n)


def version_strings_match(h):
    """Every describe string of the manifest's repositories is in the text of a version event of the window."""
    rows = [r for r in h.evidence("manifest_versions") if r]
    wanted = rows[0].get("describe", []) if rows else []
    texts = [e.text for e in h.window.ev if ".version." in e.name]
    if not wanted or not texts:
        return (False, f"{len(wanted)} manifest describe strings, {len(texts)} version events")
    missing = [d for d in wanted if not any(d in t for t in texts)]
    return (not missing, f"{len(texts)} version events; manifest strings not found: {missing}")






FIVE = ("cmdDisp", "health", "seqSched", "schedSeq", "systemResources")
SPAN_RECORDS = ("at least one record from each of CdhCore.cmdDisp, CdhCore.health, Ref.seqSched, Ref.schedSeq and "
                "Ref.systemResources lies inside the override span, 1 s clear of both ends")
SPAN_BASE = ("every event and channel record of CdhCore.cmdDisp, CdhCore.health, Ref.seqSched, Ref.schedSeq and "
             "Ref.systemResources time-tagged more than 1 s after the first TimeBaseSet and more than 1 s before "
             "TimeBaseCleared carries TB_SC_TIME, and no record of the window more than 1 s outside that span does")
SPAN_SECONDS = ("FSW seconds of every record in the override span lie between those of the last record before "
                "the span and the first record after it")


def no_start_up(h):
    rows = h.window.events("prmDb.PrmFileLoadComplete")
    return (not rows, f"{len(rows)} start-up events")


def override_span(h):
    """(first TimeBaseSet, TimeBaseCleared) FSW times of the window, or None."""
    sets = h.window.events("timeBaseSetter.TimeBaseSet")
    clears = h.window.events("timeBaseSetter.TimeBaseCleared")
    return (sets[0].fsw, clears[-1].fsw) if sets and clears else None


def records_of(h, instances=FIVE):
    rows = list(h.window.ev) + list(h.window.ch)
    return [r for r in rows if r.short.split(".")[0] in instances]


def span_has_records(h):
    span = override_span(h)
    if span is None:
        return (False, "no override span")
    inside = {r.short.split(".")[0] for r in records_of(h) if span[0] + 1 < r.fsw < span[1] - 1}
    return (inside == set(FIVE), f"instances inside: {sorted(inside)}")


def span_base(h):
    span = override_span(h)
    if span is None:
        return (False, "no override span")
    inside = [r for r in records_of(h) if span[0] + 1 < r.fsw < span[1] - 1]
    outside = [r for r in list(h.window.ev) + list(h.window.ch) if r.fsw < span[0] - 1 or r.fsw > span[1] + 1]
    wrong_in = [r.line for r in inside if r.base != TB["TB_SC_TIME"]]
    wrong_out = [r.line for r in outside if r.base == TB["TB_SC_TIME"]]
    return (bool(inside) and not wrong_in and not wrong_out,
            f"{len(inside)} inside, {len(wrong_in)} not TB_SC_TIME; {len(outside)} outside, {len(wrong_out)} TB_SC_TIME")


def span_seconds(h):
    span = override_span(h)
    if span is None:
        return (False, "no override span")
    rows = sorted(list(h.window.ev) + list(h.window.ch), key=lambda r: r.fsw)
    before = [r.fsw for r in rows if r.fsw < span[0] and r.base != TB["TB_SC_TIME"]]
    after = [r.fsw for r in rows if r.fsw > span[1] and r.base != TB["TB_SC_TIME"]]
    inside = [r.fsw for r in rows if r.base == TB["TB_SC_TIME"]]
    if not (before and after and inside):
        return (False, f"{len(before)} before, {len(inside)} inside, {len(after)} after")
    return (max(before) <= min(inside) and max(inside) <= min(after),
            f"before <= {max(before):.3f}, inside {min(inside):.3f}..{max(inside):.3f}, after >= {min(after):.3f}")


def second_set_base(h):
    sets = h.window.events("timeBaseSetter.TimeBaseSet")
    return (len(sets) >= 2 and sets[1].base == TB["TB_SC_TIME"], f"bases {[e.base for e in sets]}")






def row1_after_its_set_row(h):
    """Row 1 is set in W1 and run in W2: both times are FSW times of the same start."""
    run = run_time(h, 1, "W2")
    sent = command_time(h, "seqSched.SET_ROW", 1, "W1")
    if run is None or sent is None:
        return (False, f"run {run}, command {sent}")
    return (27 <= run - sent <= 33, f"{run - sent:.3f} s")


def due_seconds(row):
    """Seconds of a NextTimeDue sample (the recorder writes the Fw.TimeValue as text)."""
    found = re.search(r"'seconds': '?(\d+)", str(row.raw))
    return int(found.group(1)) if found else None


def elapsed_of(bench, action="next_due"):
    """The plan's `elapsed`: 1000000 s less the delay left at the newest NextTimeDue sample (0 in a dry run)."""
    rows = bench.action(action)
    if not rows or not rows[-1]:
        return 0
    found = re.search(r"'seconds': '?(\d+)", rows[-1]["value"])
    return 1000000 - (int(found.group(1)) - rows[-1]["sec"])


def elapsed_evidence(h, action="next_due"):
    """(elapsed, FSW seconds of the sample) of the test's first reading."""
    rows = [r for r in h.evidence(action) if r]
    if not rows:
        return None
    found = re.search(r"'seconds': '?(\d+)", rows[0]["value"])
    return (1000000 - (int(found.group(1)) - rows[0]["sec"]), rows[0]["sec"] + rows[0]["usec"] / 1e6)


def elapsed_since_ref_start(h):
    got, start = elapsed_evidence(h), [r for r in h.evidence("ref_start") if r]
    if got is None or not start:
        return (False, "no evidence")
    want = got[1] - start[0]["start"]
    return (abs(got[0] - want) <= 3, f"elapsed {got[0]} s, sample less process start {want:.3f} s")


def not_restarted(h):
    """No image start-up event later than the bench-recorded start of the process (2 s for the start itself)."""
    start = [r for r in h.evidence("ref_start") if r]
    if not start:
        return (False, "no evidence")
    later = [e.line for e in h.record.events
             if e.short == "prmDb.PrmFileLoadComplete" and e.fsw > start[0]["start"] + 2 and e.t <= h.window.end_marker.t]
    return (not later, f"start-up events after the recorded start: {later}")


def due_samples_after_row_set(h):
    sets = [e for e in h.window.events("seqSched.RowSet") if e.args.get("row") == 0]
    if not sets:
        return []
    return [c for c in h.window.channel("seqSched.NextTimeDue") if c.fsw > sets[0].fsw]


def enough_due_samples(h):
    rows = due_samples_after_row_set(h)
    span = rows[-1].fsw - rows[0].fsw if rows else 0
    return (len(rows) >= 3 and span >= 4, f"{len(rows)} samples over {span:.3f} s")


def due_steady(h):
    seconds = [due_seconds(c) for c in due_samples_after_row_set(h)]
    return (bool(seconds) and None not in seconds and max(seconds) - min(seconds) <= 1, f"seconds {seconds}")




def first_set_of_start(h, base):
    """This start's first TimeBaseSet(base) event at or before the end of the window, or None."""
    epoch = h.window.start_marker.epoch
    rows = [e for e in h.record.events if e.epoch == epoch and e.short == "timeBaseSetter.TimeBaseSet"
            and e.args.get("timeBase") == base and e.t <= h.window.end_marker.t]
    return rows[0] if rows else None


def first_set_recorded(base):
    def judge(h):
        row = first_set_of_start(h, base)
        return (row is not None, f"first TimeBaseSet({base}): {row.line if row else None}")
    return judge


def elapsed_since_first_set(base):
    def judge(h):
        got, row = elapsed_evidence(h), first_set_of_start(h, base)
        if got is None or row is None:
            return (False, "no evidence")
        want = got[1] - row.fsw
        return (abs(got[0] - want) <= 3, f"elapsed {got[0]} s, sample less first report {want:.3f} s")
    return judge


def due_steady_across_report(h):
    """NextTimeDue seconds before and after the window's TimeBaseSet differ by at most 1 s."""
    sets = h.window.events("timeBaseSetter.TimeBaseSet")
    rows = h.window.channel("seqSched.NextTimeDue")
    if not sets or not rows:
        return (False, f"{len(sets)} reports, {len(rows)} samples")
    after = [due_seconds(c) for c in rows if c.fsw > sets[0].fsw + 1]
    before = [due_seconds(c) for c in h.windows["W1"].channel("seqSched.NextTimeDue")][-1:]
    values = before + after
    return (bool(before) and bool(after) and None not in values and max(values) - min(values) <= 1,
            f"before {before}, after {after}")


def run_after_w1_sample(low, high):
    def judge(h):
        got, run = elapsed_evidence(h), run_time(h, 0, "W3")
        if got is None or run is None:
            return (False, f"sample {got}, run {run}")
        return (low <= run - got[1] <= high, f"{run - got[1]:.3f} s")
    return judge




def last_before_marker(channel, marker, value):
    """Last sample of a channel time-tagged before the inner marker event equals value."""
    def judge(h):
        marks = [e for e in h.window.events("cmdDisp.NoOpStringReceived") if e.args.get("id") == marker]
        if not marks:
            return (False, f"marker {marker} absent")
        rows = [c for c in h.window.channel(channel) if c.fsw < marks[0].fsw]
        return (bool(rows) and rows[-1].value == value, f"last value {rows[-1].value if rows else None}")
    return judge


def row_event_time(h, name, row):
    rows = [e for e in h.window.events(name) if e.args.get("row") == row]
    return rows[0].fsw if rows else None


def run_gap(first, second, low):
    def judge(h):
        a, b = run_time(h, first), run_time(h, second)
        if a is None or b is None:
            return (False, f"runs {a}, {b}")
        return (b - a >= low, f"{b - a:.3f} s")
    return judge


def sample_while_running(row, waiting):
    """A SeqSched sample between SeqRun(row) and SeqCompleted(row) holds RowsWaiting == waiting and RunningRow == row."""
    def judge(h):
        start, end = row_event_time(h, "seqSched.SeqRun", row), row_event_time(h, "seqSched.SeqCompleted", row)
        if start is None or end is None:
            return (False, f"run {start}, completed {end}")
        wait = {c.fsw: c.value for c in h.window.channel("seqSched.RowsWaiting") if start < c.fsw < end}
        running = {c.fsw: c.value for c in h.window.channel("seqSched.RunningRow") if start < c.fsw < end}
        hits = [t for t in wait if wait[t] == waiting and running.get(t) == row]
        return (bool(hits), f"{len(wait)} samples in the span, {len(hits)} matching")
    return judge




def abs_row(bench, offset, file_name, policy):
    """SET_ROW time and trailing arguments of an ABSOLUTE row due `offset` s after the newest recorded event."""
    return [time_value("TB_WORKSTATION_TIME", fsw_now(bench) + offset), "ABSOLUTE", file_name, policy]




YEAR_2100 = 4102444800






def settle(bench):
    """The plan's `finally` step: let a sequence left running by a failed procedure end before the next test."""
    try:
        bench.wait_event("schedSeq.CS_SequenceComplete", 25)
    except Exception:  # noqa: BLE001 - the wait is pacing only; the next test's IDLE require judges the state
        pass


def samples_b(h):
    counts = {name: len(w.channel(SAMPLE_B)) for name, w in h.windows.items()}
    return (all(n >= 2 for n in counts.values()), f"SeqSchedB samples per window: {counts}")






def first_set_before_test(base):
    """This start's first TimeBaseSet(base) is recorded before the opening marker of the window."""
    def judge(h):
        row = first_set_of_start(h, base)
        return (row is not None and row.t < h.window.start_marker.t, f"first TimeBaseSet({base}): "
                f"{row.line if row else None}")
    return judge




def bracket_both(bench):
    bracket(bench)
    bench.wait_samples(SAMPLE_B, 2)






def up_at_marker(seconds):
    """The Ref process had run at least `seconds` when the window opened (bench-recorded start)."""
    def judge(h):
        start = [r for r in h.evidence("ref_start") if r]
        if not start:
            return (False, "no evidence")
        up = h.window.start_marker.fsw - start[0]["start"]
        return (up >= seconds, f"up {up:.3f} s")
    return judge


def run_after_restart(row, low, high):
    """Row run between low and high s after the start of the Ref process recorded after the restart."""
    def judge(h):
        start, run = [r for r in h.evidence("ref_start") if r], run_time(h, row)
        if len(start) < 2 or run is None:
            return (False, f"{len(start)} recorded starts, run {run}")
        return (low <= run - start[-1]["start"] <= high, f"{run - start[-1]['start']:.3f} s")
    return judge


T_BASE_FILE = "seq_sched.tbl"
T_TRUNC_FILE = "t_trunc.tbl"
T_BADCRC_FILE = "t_badcrc.tbl"


def loads_around_run(h):
    loads = [e.fsw for e in h.window.events("seqSched.TableLoaded")]
    done = [e.fsw for e in h.window.events("schedSeq.CS_SequenceComplete") if e.args.get("fileName") == "long.bin"]
    run = run_time(h, 0)
    if len(loads) != 2 or not done or run is None:
        return (False, f"loads {loads}, complete {done}, run {run}")
    return (loads[1] < done[0] and loads[1] > run, f"loads {loads}, run {run}, complete {done[0]}")




def file_unchanged(action):
    """The file read by a bench action has the same digest at its first and last reading in the test."""
    def judge(h):
        rows = [r for r in h.evidence(action) if r]
        return (len(rows) >= 2 and rows[0]["sha256"] == rows[-1]["sha256"] and rows[0]["size"] > 0,
                f"{len(rows)} readings: {[r['sha256'][:8] for r in rows]}")
    return judge






FIELDS = ("timeBase", "seconds", "useconds", "mode", "fileName", "busyPolicy", "enabled")


def t_base():
    """Table T-BASE as the plan authors it: 16 rows, all disabled, ABSOLUTE, year 2100, short.bin."""
    return [{"timeBase": "TB_WORKSTATION_TIME", "seconds": YEAR_2100, "useconds": 0, "mode": "ABSOLUTE",
             "fileName": "short.bin", "busyPolicy": "WAIT_FOR_RUNNING", "enabled": False} for _ in range(16)]


def t_fields():
    """Table T-FIELDS: T-BASE with rows 14 and 15 holding the other value of every field."""
    rows = t_base()
    rows[14] = {"timeBase": "TB_SC_TIME", "seconds": YEAR_2100, "useconds": 0, "mode": "ABSOLUTE",
                "fileName": "short2.bin", "busyPolicy": "CANCEL_RUNNING", "enabled": True}
    rows[15] = {"timeBase": "TB_NONE", "seconds": 1000000, "useconds": 0, "mode": "START_RELATIVE",
                "fileName": "short.bin", "busyPolicy": "SKIP_WHEN_RUNNING", "enabled": False}
    return rows


def differing(got, want, fields, skip=()):
    """Rows and fields in which the rows read differ from the rows wanted."""
    if got is None or len(got) != len(want):
        return [f"{None if got is None else len(got)} rows, {len(want)} wanted"]
    return [f"row {i} {f}: {got[i].get(f)!r} != {want[i][f]!r}" for i in range(len(want)) for f in fields
            if (i, f) not in skip and got[i].get(f) != want[i][f]]


def product_is(want, states=None, skip=()):
    """The test's last decoded data product holds the wanted rows (and row states) in index order."""
    def judge(h):
        rows = [r for r in h.evidence("read_dp") if r]
        if not rows or rows[-1]["rows"] is None:
            return (False, f"no decoded product: {rows[-1]['error'] if rows else 'no evidence'}")
        got = rows[-1]["rows"]
        wrong = differing(got, want, FIELDS, skip)
        if states is not None and not wrong:
            wrong = [f"row {i} state: {got[i].get('state')!r} != {states[i]!r}" for i in range(len(want))
                     if got[i].get("state") != states[i]]
        return (not wrong, f"{rows[-1]['file']}: {wrong[:4] if wrong else 'as wanted'}")
    return judge


def new_product(h):
    """The product decoded was written in this test: more product files after the dump than before it."""
    before = [r for r in h.evidence("count_dp") if r]
    after = [r for r in h.evidence("read_dp") if r]
    return (bool(before) and bool(after) and after[-1]["files"] > before[0]["files"],
            f"files before {[r['files'] for r in before]}, after {[r['files'] for r in after]}")








# Fields of T-FINAL the test sets from the FSW clock: judged for being in the past, not for a value
PAST_TIMES = ((6, "seconds"), (8, "seconds"))


def t_final():
    """Table T-FINAL: T-BASE with row 5 short2.bin SKIP_WHEN_RUNNING enabled, rows 6 and 7 enabled."""
    rows = t_base()
    rows[5].update({"fileName": "short2.bin", "busyPolicy": "SKIP_WHEN_RUNNING", "enabled": True})
    rows[6]["enabled"] = True
    rows[7]["enabled"] = True
    return rows


def table_is_final(h):
    rows = [r for r in h.evidence("read_table") if r]
    if not rows or not rows[-1]["valid"]:
        return (False, f"table file not valid: {rows[-1]['error'] if rows else 'no evidence'}")
    got = rows[-1]["rows"]
    wrong = differing(got, t_final(), FIELDS, PAST_TIMES)
    if not wrong:
        wrong = [f"row {i} carries a state" for i in range(16) if "state" in got[i]]
    if not wrong:
        wrong = [f"row {i} time {got[i]['seconds']} is not a past FSW time" for i, _ in PAST_TIMES
                 if not 0 < got[i]["seconds"] < h.window.end_marker.fsw]
    return (not wrong, f"{wrong[:4] if wrong else 'as wanted'}")


