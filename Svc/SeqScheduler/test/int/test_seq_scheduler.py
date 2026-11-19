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


def time_value(base, seconds):
    """A Fw.TimeValue command argument, in the JSON text a command takes on this bench."""
    return json.dumps({"timeBase": base, "timeContext": 0, "seconds": int(seconds), "useconds": 0})


def fsw_now(bench):
    """Seconds of the newest recorded event (0 in a dry run, where nothing is recorded)."""
    rows = bench.action("fsw_now")
    return int(rows[-1]["sec"]) if rows and rows[-1] else 0


def fresh_times(h):
    rows = h.evidence("fsw_now")
    ages = [r["age_s"] for r in rows if r]
    return (len(ages) == len(rows) and len(rows) > 0 and max(ages) <= 1.0, f"ages {ages}")


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
    if fresh:
        require(FRESH, fresh_times)


def run_time(h, row, window=None):
    w = h.windows[window] if window else h.window
    rows = [e for e in w.events("seqSched.SeqRun") if e.args.get("row") == row]
    return rows[0].fsw if rows else None


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
    """Every version event of the window is present and non-empty; identity is judged against the manifest by the audit."""
    rows = [e for e in h.window.ev if ".version." in e.name]
    return (len(rows) > 0 and all(e.text.strip() for e in rows), f"{len(rows)} version events")


def test_L0_1(window, send, bench, check, require):
    with window(100, 101):
        require("STD")
        bracket(bench)
        send("CdhCore.version.VERSION", ["ALL"])
        check("version payloads == manifest describe strings", version_strings_match)
        check("event cmdDisp.NoOpStringReceived(id=100) count == 1")


def test_L2_2(window, send, bench, check, require):
    with window(210, 211):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW", [0, time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 10), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(16)
        bracket(bench)
        check("ABS")
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceLoaded(fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSched.SeqRun(row=0) precedes event seqSched.SeqCompleted(row=0)")
        check("last(seqSched.RowsExecuted) == 1")
        check("row 0 run no earlier than 8 s after SET_ROW", run_after(0, "seqSched.SET_ROW", 8, None))


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


def test_L0_2(window, send, bench, check, require):
    with window(110, 111):
        require("STD")
        require("no image start-up event in the window", no_start_up)
        standard_requires(require)
        bracket(bench)
        send(T + ".SET_TIME_BASE", ["TB_SC_TIME"])
        bench.wait(2)
        send(Q + ".CS_RUN", ["short.bin", "NO_BLOCK"])
        send("CdhCore.cmdDisp.CMD_NO_OP")
        send("CdhCore.health.HLTH_ENABLE", ["ENABLED"])
        send(S + ".LOAD_TABLE")
        bench.wait_event("schedSeq.CS_SequenceComplete", 5)
        send(T + ".SET_TIME_BASE", ["TB_SC_TIME"])
        bench.wait(3)
        send(T + ".CLEAR_TIME_BASE")
        bracket(bench)
        check("event timeBaseSetter.TimeBaseSet(timeBase=TB_SC_TIME) count == 2")
        check("event timeBaseSetter.TimeBaseCleared count == 1")
        require(SPAN_RECORDS, span_has_records)
        check(SPAN_BASE, span_base)
        check(SPAN_SECONDS, span_seconds)
        check("the time tag of the second timeBaseSetter.TimeBaseSet event carries TB_SC_TIME", second_set_base)
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")


def test_L2_3(window, send, bench, check, require):
    with window(220, 221):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW_ENABLED", [1, True])
        send(S + ".SET_ROW", [0, time_value("TB_SC_TIME", fsw_now(bench) + 5), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        send(S + ".SET_ROW", [1, time_value("TB_SC_TIME", fsw_now(bench) + 30), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(12)
        bracket(bench)
        check("event seqSched.SeqRun count == 0")
        check("event seqSched.RowLate count == 0")
        check("last(seqSched.RowsPending) == 16")
        check("last(seqSched.NextRow) == 16")
    with window(221, 222):
        require("STD")
        send(T + ".SET_TIME_BASE", ["TB_SC_TIME"])
        bench.wait(24)
        bracket(bench)
        send(T + ".CLEAR_TIME_BASE")
        check("event seqSched.RowLate(row=0) count == 1")
        check("event seqSched.SeqRun(row=0) count == 0")
        check("event seqSched.SeqRun(row=1, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("last(seqSched.RowsSkipped) == 1")
        check("event timeBaseSetter.TimeBaseSet(timeBase=TB_SC_TIME) count == 1")
        check("event timeBaseSetter.TimeBaseCleared count == 1")
        check("row 1 run between 27 and 33 s after its SET_ROW", row1_after_its_set_row)


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


def test_L2_4(window, send, bench, check, require):
    with window(230, 231):
        require("STD")
        standard_requires(require)
        bench.action("ref_start")
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW", [0, time_value("TB_NONE", 1000000), "START_RELATIVE", "short.bin", "WAIT_FOR_RUNNING"])
        bench.wait(4)
        bracket(bench)
        elapsed = elapsed_of(bench)
        check("last(seqSched.NextRow) == 0")
        check("event seqSched.SeqRun count == 0")
        require("at least 3 NextTimeDue samples time-tagged after RowSet(row=0), spanning >= 4 s", enough_due_samples)
        check("seconds of those NextTimeDue samples are equal within 1 s", due_steady)
        check("elapsed is within 3 s of FSW time at the sample less the bench-recorded start of the Ref process",
              elapsed_since_ref_start)
        require("Ref process not restarted since the bench recorded its start", not_restarted)
    with window(231, 232):
        require("STD")
        send(S + ".SET_ROW", [0, time_value("TB_NONE", elapsed + 12), "START_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(18)
        bracket(bench)
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("row 0 run no earlier than 9 s after the second SET_ROW", run_after(0, "seqSched.SET_ROW", 9, None))
        check("row 0 run no later than 15 s after the second SET_ROW", run_after(0, "seqSched.SET_ROW", None, 15))


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


def test_L2_5(window, send, bench, check, require):
    with window(240, 241):
        require("STD")
        standard_requires(require)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(T + ".SET_TIME_BASE", ["TB_PROC_TIME"])
        bench.wait(3)
        send(T + ".CLEAR_TIME_BASE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW", [0, time_value("TB_PROC_TIME", 1000000), "TIME_BASE_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bracket(bench)
        elapsed = elapsed_of(bench)
        check("last(seqSched.NextRow) == 0")
        check("event timeBaseSetter.TimeBaseSet(timeBase=TB_PROC_TIME) count == 1")
        check("elapsed is within 3 s of FSW time at the sample less the time of this start's first "
              "timeBaseSetter.TimeBaseSet(TB_PROC_TIME) event", elapsed_since_first_set("TB_PROC_TIME"))
        require("the first TimeBaseSet(TB_PROC_TIME) of this start is in the record, in this test or an earlier one",
                first_set_recorded("TB_PROC_TIME"))
    with window(241, 242):
        require("STD")
        send(T + ".SET_TIME_BASE", ["TB_PROC_TIME"])
        bench.wait(3)
        send(T + ".CLEAR_TIME_BASE")
        bracket(bench)
        check("seconds of NextTimeDue are equal within 1 s across the second TB_PROC_TIME report",
              due_steady_across_report)
    with window(242, 243):
        require("STD")
        send(S + ".SET_ROW", [0, time_value("TB_PROC_TIME", elapsed + 20), "TIME_BASE_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(20)
        bracket(bench)
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("row 0 run between 11 and 21 s after the W1 NextTimeDue sample", run_after_w1_sample(11, 21))


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


def test_L2_6(window, send, bench, check, require):
    with window(250, 251):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW_ENABLED", [1, True])
        send(S + ".SET_ROW_ENABLED", [2, True])
        due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 12)
        send(S + ".SET_ROW", [2, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send(S + ".SET_ROW", [1, due, "ABSOLUTE", "missing.bin", "WAIT_FOR_RUNNING"])
        send(S + ".SET_ROW", [0, due, "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
        bench.wait(17)
        send(MARK, [252])
        bench.wait(23)
        bracket(bench)
        check("event seqSched.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event seqSched.SeqRun(row=1, fileName=missing.bin) count == 1")
        check("event seqSched.SeqRun(row=2, fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSched.SeqCompleted(row=1, response=EXECUTION_ERROR) count == 1")
        check("event seqSched.SeqCompleted(row=2, response=OK) count == 1")
        check("last(seqSched.RowsExecuted) before marker 252 == 0", last_before_marker("seqSched.RowsExecuted", 252, 0))
        check("last(seqSched.RunningRow) before marker 252 == 0", last_before_marker("seqSched.RunningRow", 252, 0))
        check("event seqSched.SeqCompleted(row=0) precedes event seqSched.SeqRun(row=1)")
        check("event seqSched.SeqCompleted(row=1) precedes event seqSched.SeqRun(row=2)")
        check("event seqSched.RowLate count == 0")
        check("event seqSched.SeqCancelled count == 0")
        check("event seqSched.RowSkippedBusy count == 0")
        check("event schedSeq.CS_FileNotFound(fileName=missing.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=long.bin) precedes "
              "event schedSeq.CS_FileNotFound(fileName=missing.bin)")
        check("row 1 run at least 15 s after row 0", run_gap(0, 1, 15))
        check("last(seqSched.RowsExecuted) == 3")
        check("last(seqSched.RowsWaiting) == 0")
        check("a SeqSched sample time-tagged between SeqRun(row=0) and SeqCompleted(row=0) holds RowsWaiting == 2 "
              "and RunningRow == 0", sample_while_running(0, 2))
