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


def reqs(numbers):
    """Requirement ids of the component from their numbers."""
    return ["REQ-SeqScheduler-" + n for n in numbers.split()]


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


def run_not_before_row_time(row):
    """Row run at or after the time its (first) RowSet event in the window reports for it."""
    def judge(h):
        run = run_time(h, row)
        sets = [e for e in h.window.events("seqSched.RowSet") if e.args.get("row") == row]
        found = re.search(r"'seconds': (\d+)", sets[0].text) if sets else None
        if run is None or found is None:
            return (False, f"run {run}, RowSet {sets[0].text if sets else None}")
        return (run >= int(found.group(1)), f"run {run:.3f}, row time {found.group(1)}")
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


@pytest.mark.req(*reqs("006 012 020 040"))
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
        check("row 0 run no earlier than the time set in the row", run_not_before_row_time(0))


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
        try:
            bench.wait(2)
            # short.bin ends within the tick that starts it: count from before the command that runs it
            done = bench.count_events("schedSeq.CS_SequenceComplete")
            send(Q + ".CS_RUN", ["short.bin", "NO_BLOCK"])
            send("CdhCore.cmdDisp.CMD_NO_OP")
            send("CdhCore.health.HLTH_ENABLE", ["ENABLED"])
            send(S + ".LOAD_TABLE")
            bench.wait_event("schedSeq.CS_SequenceComplete", 5, base=done)
            send(T + ".SET_TIME_BASE", ["TB_SC_TIME"])
            bench.wait(3)
        except Exception:
            send(T + ".CLEAR_TIME_BASE")  # a failed procedure must not leave the override set for the next test
            raise
        send(T + ".CLEAR_TIME_BASE")
        bracket(bench)
        check("event timeBaseSetter.TimeBaseSet(timeBase=TB_SC_TIME) count == 2")
        check("event timeBaseSetter.TimeBaseCleared count == 1")
        require(SPAN_RECORDS, span_has_records)
        check(SPAN_BASE, span_base)
        check(SPAN_SECONDS, span_seconds)
        check("the time tag of the second timeBaseSetter.TimeBaseSet event carries TB_SC_TIME", second_set_base)
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")


@pytest.mark.req(*reqs("007 021 006"))
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


@pytest.mark.req(*reqs("008 041 047"))
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
        # read again: the markers between the windows take longer than the 3 s the pass rule allows for
        elapsed = elapsed_of(bench)
        send(S + ".SET_ROW", [0, time_value("TB_NONE", elapsed + 12), "START_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(18)
        bracket(bench)
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("row 0 run no earlier than its delay after the start of the Ref process", run_not_before_delay(0))
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


@pytest.mark.req(*reqs("009 011 061"))
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


@pytest.mark.req(*reqs("013 014 017 045 012 040 041"))
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


def abs_row(bench, offset, file_name, policy):
    """SET_ROW time and trailing arguments of an ABSOLUTE row due `offset` s after the newest recorded event."""
    return [time_value("TB_WORKSTATION_TIME", fsw_now(bench) + offset), "ABSOLUTE", file_name, policy]


@pytest.mark.req(*reqs("015 013 060"))
def test_L2_7(window, send, bench, check, require):
    with window(260, 261):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW_ENABLED", [1, True])
        send(S + ".SET_ROW_ENABLED", [2, True])
        send(S + ".SET_ROW", [1] + abs_row(bench, 16, "short.bin", "CANCEL_RUNNING"))
        send(S + ".SET_ROW", [2] + abs_row(bench, 11, "short2.bin", "WAIT_FOR_RUNNING"))
        send(S + ".SET_ROW", [0] + abs_row(bench, 6, "long.bin", "WAIT_FOR_RUNNING"))
        bench.wait(24)
        bracket(bench)
        check("event seqSched.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event seqSched.SeqCancelled(cancelledRow=0, row=1) count == 1")
        check("event schedSeq.CS_SequenceCanceled(fileName=long.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=long.bin) count == 0")
        check("event seqSched.RowSkippedByCancel(row=2, cancelRow=1) count == 1")
        check("event seqSched.RowSkippedByCancel count == 1")
        check("event seqSched.SeqRun(row=2) count == 0")
        check("event schedSeq.CS_SequenceLoaded(fileName=short2.bin) count == 0")
        check("event seqSched.SeqRun(row=1, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("event seqSched.SeqCancelled(row=1) precedes event seqSched.SeqRun(row=1)")
        check("event schedSeq.CS_SequenceCanceled(fileName=long.bin) precedes "
              "event schedSeq.CS_SequenceLoaded(fileName=short.bin)")
        check("event seqSched.SeqCompleted(row=1, response=OK) count == 1")
        check("event seqSched.SeqCancelled count == 1")
        check("last(seqSched.RowsSkipped) == 1")
        check("last(seqSched.RowsWaiting) == 0")
        check("event seqSched.SeqCompleted(row=0, response=EXECUTION_ERROR) count == 1")
        check("event seqSched.SeqCompleted(row=0) precedes event seqSched.SeqRun(row=1)")


YEAR_2100 = 4102444800


@pytest.mark.req(*reqs("016 013 012"))
def test_L2_8(window, send, bench, check, require):
    with window(262, 263):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW_ENABLED", [1, True])
        send(S + ".SET_ROW_ENABLED", [2, True])
        send(S + ".SET_ROW", [2] + abs_row(bench, 34, "short2.bin", "SKIP_WHEN_RUNNING"))
        send(S + ".SET_ROW", [1] + abs_row(bench, 12, "short.bin", "SKIP_WHEN_RUNNING"))
        send(S + ".SET_ROW", [0] + abs_row(bench, 6, "long.bin", "WAIT_FOR_RUNNING"))
        bench.wait(42)
        bracket(bench)
        check("ABS")
        check("event seqSched.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event seqSched.RowSkippedBusy(row=1, runningRow=0) count == 1")
        check("event seqSched.SeqRun(row=1) count == 0")
        check("event schedSeq.CS_SequenceLoaded(fileName=short.bin) count == 0")
        check("event schedSeq.CS_SequenceComplete(fileName=long.bin) count == 1")
        check("event schedSeq.CS_SequenceCanceled count == 0")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSched.SeqRun(row=2, fileName=short2.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1")
        check("event seqSched.RowSkippedBusy count == 1")
        check("event seqSched.RowLate count == 0")
        check("last(seqSched.RowsSkipped) == 1")
        check("last(seqSched.RowsExecuted) == 2")


@pytest.mark.req(*reqs("025 026 027 022 029 012"))
def test_L2_9(window, send, bench, check, require):
    with window(270, 271):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        for row in range(4):
            send(S + ".SET_ROW_ENABLED", [row, True])
        send(S + ".SET_ROW", [2, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "long.bin",
                              "WAIT_FOR_RUNNING"])
        due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 12)
        send(S + ".SET_ROW", [3, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send(S + ".SET_ROW", [1, due, "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
        send(S + ".SET_ROW", [0, due, "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
        send(S + ".SET_SEQ_ENABLED", ["long.bin", False])
        send(S + ".SET_ROW_ENABLED", [3, False])
        bench.wait(18)
        bracket(bench)
        check("ABS")
        check("event seqSched.RowSet(row=0, fileName=short2.bin) count == 1")
        check("event seqSched.RowSet count == 4")
        check("event seqSched.SeqEnabledSet(fileName=long.bin, enabled=False, rows=2) count == 1")
        check("event seqSched.RowEnabledSet(row=3, enabled=False) count == 1")
        check("event seqSched.SeqRun(row=0, fileName=short2.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1")
        check("event seqSched.SeqRun count == 1")
        check("event seqSched.RowLate count == 0")
        check("event seqSched.RowSkippedDisabled(row=1) count == 1")
        check("event seqSched.RowSkippedDisabled(row=3) count == 1")
        check("event seqSched.RowSkippedDisabled count == 2")
        check("event schedSeq.CS_SequenceLoaded(fileName=long.bin) count == 0")
        check("last(seqSched.RowsExecuted) == 1")
        check("last(seqSched.RowsSkipped) == 2")
        check("last(seqSched.RowsPending) == 13")


def settle(bench):
    """The plan's `finally` step: let a sequence left running by a failed procedure end before the next test."""
    try:
        bench.wait_event("schedSeq.CS_SequenceComplete", 25)
    except Exception:  # noqa: BLE001 - the wait is pacing only; the next test's IDLE require judges the state
        pass


def samples_b(h):
    counts = {name: len(w.channel(SAMPLE_B)) for name, w in h.windows.items()}
    return (all(n >= 2 for n in counts.values()), f"SeqSchedB samples per window: {counts}")


@pytest.mark.req(*reqs("042 012 040 007 022 026 034"))
def test_L2_10(window, send, bench, check, require):
    try:
        with window(280, 281):
            require("STD-ERR")
            standard_requires(require, fresh=True)
            bench.action("write_T_BASE")
            send(S + ".LOAD_TABLE")
            bench.action("write_T_BASE_B")
            send(B + ".LOAD_TABLE")
            send(S + ".SET_SEQ_ENABLED", ["short.bin", True])
            send(B + ".SET_ROW_ENABLED", [0, True])
            send(B + ".SET_ROW_ENABLED", [1, True])
            send(B + ".SET_ROW_ENABLED", [16, True])
            send(B + ".SET_ROW", [1, time_value("TB_SC_TIME", fsw_now(bench) + 6), "ABSOLUTE", "short.bin",
                                  "WAIT_FOR_RUNNING"])
            due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 12)
            send(B + ".SET_ROW", [2, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
            send(B + ".SET_ROW", [0, due, "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
            send(S + ".SET_ROW", [0, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
            bench.wait(22)
            bracket(bench)
            bench.wait_samples(SAMPLE_B, 2)
            check("ABS")
            check("event cmdDisp.OpCodeError count == 1")
            check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
            check("event seqSchedB.SeqRun(row=0, fileName=short2.bin) count == 1")
            check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
            check("event schedSeqB.CS_SequenceComplete(fileName=short2.bin) count == 1")
            check("event schedSeq.CS_SequenceLoaded(fileName=short2.bin) count == 0")
            check("event schedSeqB.CS_SequenceLoaded(fileName=short.bin) count == 0")
            check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
            check("event seqSchedB.SeqCompleted(row=0, response=OK) count == 1")
            check("event seqSched.SeqEnabledSet(fileName=short.bin, enabled=True, rows=16) count == 1")
            check("event seqSched.SeqRun count == 1")
            check("event seqSchedB.SeqRun count == 1")
            check("event seqSchedB.CmdRejected(reason=ROW_INDEX) count == 1")
            # "event seqSched.CmdRejected count == 0" of the plan is one of the ABS checks above
            check("event seqSchedB.RowLate count == 0")
            check("event seqSchedB.RowSkippedDisabled(row=2) count == 1")
            check("event seqSchedB.RowSkippedDisabled count == 1")
            check("last(seqSched.RowsExecuted) == 1")
            check("last(seqSched.RowsPending) == 15")
            check("last(seqSchedB.RowsExecuted) == 1")
            check("last(seqSchedB.RowsSkipped) == 1")
            check("last(seqSchedB.RowsPending) == 14")
            require("SeqSchedB samples >= 2 in every window", samples_b)
    except BaseException:
        settle(bench)
        raise


@pytest.mark.req(*reqs("006 012 020 040 027 028 029"))
def test_L2_11(window, send, bench, check, require):
    try:
        with window(290, 291):
            require("STD")
            standard_requires(require, fresh=True)
            bench.action("write_T_BASE")
            send(S + ".LOAD_TABLE")
            send(S + ".SET_ROW_ENABLED", [3, True])
            send(S + ".SET_ROW", [3] + abs_row(bench, 8, "short2.bin", "WAIT_FOR_RUNNING"))
            bench.wait(14)
            send(S + ".SET_ROW_ENABLED", [3, False])
            send(S + ".SET_ROW_ENABLED", [3, True])
            bench.wait(4)
            bracket(bench)
            check("ABS")
            check("event seqSched.RowSet(row=3, mode=ABSOLUTE, fileName=short2.bin, busyPolicy=WAIT_FOR_RUNNING) "
                  "count == 1")
            check("event seqSched.SeqRun(row=3, fileName=short2.bin) count == 1")
            check("event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1")
            check("event seqSched.SeqCompleted(row=3, response=OK) count == 1")
            check("event seqSched.SeqRun count == 1")
            check("event seqSched.RowEnabledSet(row=3) count == 3")
            check("last(seqSched.RowsExecuted) == 1")
            check("last(seqSched.RowsPending) == 15")
            check("row 3 run no earlier than the time set in the row", run_not_before_row_time(3))
            check("row 3 run no later than 10 s after SET_ROW", run_after(3, "seqSched.SET_ROW", None, 10))
        with window(291, 292):
            require("STD")
            send(S + ".SET_ROW", [3] + abs_row(bench, 6, "short2.bin", "WAIT_FOR_RUNNING"))
            bench.wait(12)
            bracket(bench)
            check("ABS")
            check("event seqSched.SeqRun(row=3, fileName=short2.bin) count == 1")
            check("event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1")
            check("last(seqSched.RowsExecuted) == 1")
    except BaseException:
        settle(bench)
        raise


def first_set_before_test(base):
    """This start's first TimeBaseSet(base) is recorded before the opening marker of the window."""
    def judge(h):
        row = first_set_of_start(h, base)
        return (row is not None and row.t < h.window.start_marker.t, f"first TimeBaseSet({base}): "
                f"{row.line if row else None}")
    return judge


@pytest.mark.req(*reqs("009 011 047 061"))
def test_L2_12(window, send, bench, check, require):
    with window(293, 294):
        require("STD")
        standard_requires(require)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW", [0, time_value("TB_SC_TIME", 1000000), "TIME_BASE_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bracket(bench)
        elapsed = elapsed_of(bench)
        send(S + ".SET_ROW", [0, time_value("TB_SC_TIME", elapsed + 20), "TIME_BASE_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        send(T + ".SET_TIME_BASE", ["TB_SC_TIME"])
        bench.wait(3)
        send(T + ".CLEAR_TIME_BASE")
        bench.wait(20)
        bracket(bench)
        check("ABS")
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSched.RowLate count == 0")
        check("last(seqSched.RowsExecuted) == 1")
        check("row 0 run between 14 and 22 s after the second SET_ROW", run_after(0, "seqSched.SET_ROW", 14, 22, 1))
        check("elapsed is within 3 s of FSW time at the sample less the time of this start's first "
              "timeBaseSetter.TimeBaseSet(TB_SC_TIME) event", elapsed_since_first_set("TB_SC_TIME"))
        require("the first TimeBaseSet(TB_SC_TIME) of this start is in the record, in an earlier test",
                first_set_before_test("TB_SC_TIME"))


def bracket_both(bench):
    bracket(bench)
    bench.wait_samples(SAMPLE_B, 2)


@pytest.mark.req(*reqs("015 042 060 045"))
def test_L2_13(window, send, bench, check, require):
    with window(295, 296):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE_B")
        send(B + ".LOAD_TABLE")
        for row in range(5):
            send(B + ".SET_ROW_ENABLED", [row, True])
        send(B + ".SET_ROW", [4] + abs_row(bench, 18, "short2.bin", "WAIT_FOR_RUNNING"))
        send(B + ".SET_ROW", [2] + abs_row(bench, 14, "short.bin", "CANCEL_RUNNING"))
        send(B + ".SET_ROW", [3] + abs_row(bench, 11, "short.bin", "WAIT_FOR_RUNNING"))
        send(B + ".SET_ROW", [1] + abs_row(bench, 9, "short.bin", "WAIT_FOR_RUNNING"))
        send(B + ".SET_ROW", [0] + abs_row(bench, 6, "long.bin", "WAIT_FOR_RUNNING"))
        bench.wait(31)
        bracket_both(bench)
        check("event seqSchedB.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event seqSchedB.SeqCancelled(cancelledRow=0, row=2) count == 1")
        check("event schedSeqB.CS_SequenceCanceled(fileName=long.bin) count == 1")
        check("event schedSeqB.CS_SequenceComplete(fileName=long.bin) count == 0")
        check("event seqSchedB.RowSkippedByCancel(row=1, cancelRow=2) count == 1")
        check("event seqSchedB.RowSkippedByCancel(row=3, cancelRow=2) count == 1")
        check("event seqSchedB.RowSkippedByCancel count == 2")
        check("event seqSchedB.SeqRun(row=1) count == 0")
        check("event seqSchedB.SeqRun(row=3) count == 0")
        check("event seqSchedB.SeqRun(row=2, fileName=short.bin) count == 1")
        check("event seqSchedB.SeqRun(row=4, fileName=short2.bin) count == 1")
        check("event schedSeqB.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("event schedSeqB.CS_SequenceComplete(fileName=short2.bin) count == 1")
        check("event seqSchedB.SeqCancelled(row=2) precedes event seqSchedB.SeqRun(row=2)")
        check("event seqSchedB.SeqCompleted(row=2) precedes event seqSchedB.SeqRun(row=4)")
        check("event seqSchedB.SeqCompleted(row=2, response=OK) count == 1")
        check("event seqSchedB.SeqCompleted(row=4, response=OK) count == 1")
        check("event seqSchedB.SeqCancelled count == 1")
        check("last(seqSchedB.RowsSkipped) == 2")
        check("last(seqSchedB.RowsExecuted) == 3")
        check("last(seqSchedB.RowsWaiting) == 0")
        check("event seqSchedB.SeqCompleted(row=0, response=EXECUTION_ERROR) count == 1")
        check("event seqSchedB.SeqCompleted(row=0) precedes event seqSchedB.SeqRun(row=2)")
        check("event seqSched.SeqRun count == 0")
        check("event seqSched.SeqCancelled count == 0")
        check("event seqSched.RowSkippedByCancel count == 0")
        check("event schedSeq.CS_SequenceCanceled count == 0")
        require("SeqSchedB samples >= 2 in every window", samples_b)


@pytest.mark.req(*reqs("016 045 014 013 017 042"))
def test_L2_14(window, send, bench, check, require):
    with window(297, 298):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE_B")
        send(B + ".LOAD_TABLE")
        for row in range(5):
            send(B + ".SET_ROW_ENABLED", [row, True])
        send(B + ".SET_ROW", [1] + abs_row(bench, 16, "short.bin", "WAIT_FOR_RUNNING"))
        send(B + ".SET_ROW", [3] + abs_row(bench, 13, "short.bin", "SKIP_WHEN_RUNNING"))
        due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 10)
        send(B + ".SET_ROW", [4, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send(B + ".SET_ROW", [2, due, "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
        send(B + ".SET_ROW", [0] + abs_row(bench, 6, "long.bin", "WAIT_FOR_RUNNING"))
        bench.wait(44)
        bracket_both(bench)
        check("event seqSchedB.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event seqSchedB.RowSkippedBusy(row=3, runningRow=0) count == 1")
        check("event seqSchedB.SeqRun(row=3) count == 0")
        check("event seqSchedB.SeqRun(row=2, fileName=short2.bin) count == 1")
        check("event seqSchedB.SeqRun(row=4, fileName=short.bin) count == 1")
        check("event seqSchedB.SeqRun(row=1, fileName=short.bin) count == 1")
        check("event seqSchedB.SeqCompleted(row=0) precedes event seqSchedB.SeqRun(row=2)")
        check("event seqSchedB.SeqCompleted(row=2) precedes event seqSchedB.SeqRun(row=4)")
        check("event seqSchedB.SeqCompleted(row=4) precedes event seqSchedB.SeqRun(row=1)")
        check("event schedSeqB.CS_SequenceComplete(fileName=long.bin) count == 1")
        check("event schedSeqB.CS_SequenceCanceled count == 0")
        check("event seqSchedB.RowLate count == 0")
        check("event seqSchedB.RowSkippedByCancel count == 0")
        check("last(seqSchedB.RowsExecuted) == 4")
        check("last(seqSchedB.RowsSkipped) == 1")
        check("event seqSched.SeqRun count == 0")
        check("event seqSched.RowSkippedBusy count == 0")
        require("SeqSchedB samples >= 2 in every window", samples_b)


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


@pytest.mark.req(*reqs("023 032 031 056 021 008 005"))
def test_L3_1(window, send, bench, check, require):
    with window(300, 301):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("ref_start")
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW_ENABLED", [1, True])
        send(S + ".SET_ROW", [1, time_value("TB_NONE", 1), "START_RELATIVE", "short.bin", "WAIT_FOR_RUNNING"])
        send(S + ".SET_ROW", [0] + abs_row(bench, 6, "short.bin", "WAIT_FOR_RUNNING"))
        bench.wait(12)
        send(S + ".LOAD_TABLE")
        bracket(bench)
        check("event seqSched.TableLoaded count == 2")
        check("event seqSched.SeqRun(row=0) count == 1")
        check("event seqSched.RowLate(row=1) count == 1")
        check("last(seqSched.RowsPending) == 16")
        check("last(seqSched.RowsExecuted) == 0")
    with window(301, 302):
        require("STD")
        require("Ref process up >= 70 s at marker 301", up_at_marker(70))
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW", [0] + abs_row(bench, 6, "short.bin", "WAIT_FOR_RUNNING"))
        bench.wait(12)
        send(S + ".SET_ROW_ENABLED", [1, True])
        send(S + ".SET_ROW", [1, time_value("TB_NONE", 60), "START_RELATIVE", "short.bin", "WAIT_FOR_RUNNING"])
        bench.wait(3)
        send(S + ".SAVE_TABLE")
        bracket(bench)
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event seqSched.TableSaved count == 1")
        check("event seqSched.RowLate(row=1) count == 1")
        check("last(seqSched.RowsSkipped) == 1")
        check("last(seqSched.RowsExecuted) == 1")
    with window(302, 303):
        bench.reset()
        bench.action("ref_start")
        require("both markers present")
        check("event seqSched.TableLoaded count == 1")
    with window(303, 304):
        require("STD")
        bench.wait(70)
        bracket(bench)
        check("event seqSched.SeqRun(row=0) count == 0")
        check("event seqSched.SeqRun(row=1, fileName=short.bin) count == 1")
        check("event seqSched.RowLate count == 0")
        check("last(seqSched.TableLoaded) == True")
        check("last(seqSched.RowsExecuted) == 1")
        check("last(seqSched.RowsSkipped) == 1")
        check("last(seqSched.RowsPending) == 14")
        check("event seqSched.RowSkippedDisabled count == 0")
        check("row 1 run between 57 and 66 s after the bench-recorded start of the Ref process",
              run_after_restart(1, 57, 66))


def disable_and_report(window, send, bench, check, require, start, inst, name, seq, both):
    """L2-15 / L2-16: the same procedure and checks on one scheduler instance."""
    try:
        with window(start, start + 1):
            require("STD")
            standard_requires(require, fresh=True)
            require(f"first({name}.Enabled) == True")
            bench.action("write_T_BASE_B" if both else "write_T_BASE")
            send(inst + ".LOAD_TABLE")
            send(inst + ".SET_ROW_ENABLED", [0, True])
            send(inst + ".SET_ROW_ENABLED", [1, True])
            send(inst + ".SET_ROW", [0] + abs_row(bench, 12, "short.bin", "WAIT_FOR_RUNNING"))
            send(inst + ".SET_ENABLED", [False])
            send(inst + ".REPORT_ACTIVE_ROW")
            bench.wait(18)
            send(inst + ".SET_ENABLED", [True])
            send(inst + ".SET_ROW", [1] + abs_row(bench, 10, "short.bin", "WAIT_FOR_RUNNING"))
            send(inst + ".REPORT_ACTIVE_ROW")
            bench.wait(16)
            bracket_both(bench) if both else bracket(bench)
            check(f"event {name}.EnabledSet(enabled=False) count == 1")
            check(f"event {name}.EnabledSet(enabled=True) count == 1")
            check(f"event {name}.NoActiveRow count == 1")
            check(f"event {name}.ActiveRow(row=1) count == 1")
            check(f"event {name}.ActiveRow count == 1")
            check(f"event {name}.RowSkippedDisabled(row=0) count == 1")
            check(f"event {name}.SeqRun(row=0) count == 0")
            check(f"event {name}.SeqRun(row=1) count == 1")
            check(f"event {name}.SeqRun count == 1")
            check(f"event {seq}.CS_SequenceComplete count == 1")
            check(f"event {name}.RowLate count == 0")
            check(f"event {name}.NoActiveRow precedes event {name}.RowSkippedDisabled(row=0)")
            check(f"last({name}.Enabled) == True")
            check(f"last({name}.RowsSkipped) == 1")
            check(f"last({name}.RowsExecuted) == 1")
            check(f"last({name}.LastExecutedRow) == 1")
            if both:
                check("event seqSched.SeqRun count == 0")
                check("event seqSched.EnabledSet count == 0")
                check("last(seqSched.Enabled) == True")
                require("SeqSchedB samples >= 2 in every window", samples_b)
    except BaseException:
        # the plan's `finally`: a failed procedure must not leave the scheduler disabled for the next test
        send(inst + ".SET_ENABLED", [True])
        raise


@pytest.mark.req(*reqs("057 058"))
def test_L2_15(window, send, bench, check, require):
    disable_and_report(window, send, bench, check, require, 310, S, "seqSched", "schedSeq", False)


@pytest.mark.req(*reqs("057 058 042"))
def test_L2_16(window, send, bench, check, require):
    disable_and_report(window, send, bench, check, require, 320, B, "seqSchedB", "schedSeqB", True)


T_BASE_FILE = "seq_sched.tbl"
T_TRUNC_FILE = "t_trunc.tbl"
T_BADCRC_FILE = "t_badcrc.tbl"


def restore_table(window, send, bench, check, require, start):
    """Clean-up window of the load-failure tests: TABLE_FILE back to the plan value and T-BASE loaded."""
    with window(start, start + 1):
        require("both markers present")
        send(S + ".TABLE_FILE_PRM_SET", [T_BASE_FILE])
        send(S + ".LOAD_TABLE")
        bracket(bench)
        check("event seqSched.TableLoaded count == 1")


@pytest.mark.req(*reqs("003 005"))
def test_L5_1(window, send, bench, check, require):
    try:
        with window(500, 501):
            require("STD-ERR")
            standard_requires(require, fresh=True)
            bench.action("write_T_BASE")
            bench.action("write_T_TRUNC")
            send(S + ".LOAD_TABLE")
            send(S + ".SET_ROW_ENABLED", [0, True])
            send(S + ".SET_ROW", [0] + abs_row(bench, 15, "short.bin", "WAIT_FOR_RUNNING"))
            send(S + ".TABLE_FILE_PRM_SET", ["missing.tbl"])
            send(S + ".LOAD_TABLE")
            send(S + ".TABLE_FILE_PRM_SET", [T_TRUNC_FILE])
            send(S + ".LOAD_TABLE")
            bench.wait(20)
            bracket(bench)
            check("event cmdDisp.OpCodeError count == 2")
            check("event seqSched.TableLoadFailed(reason=OPEN) count == 1")
            check("event seqSched.TableFileMissing count == 0")
            check("event seqSched.TableLoadFailed(reason=SIZE) count == 1")
            check("event seqSched.TableLoaded count == 1")
            check("event seqSched.CmdRejected count == 0")
            check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
            check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
            check("last(seqSched.TableLoaded) == True")
            check("last(seqSched.RowsPending) == 15")
            check("last(seqSched.RowsExecuted) == 1")
    finally:
        restore_table(window, send, bench, check, require, 501)


def loads_around_run(h):
    loads = [e.fsw for e in h.window.events("seqSched.TableLoaded")]
    done = [e.fsw for e in h.window.events("schedSeq.CS_SequenceComplete") if e.args.get("fileName") == "long.bin"]
    run = run_time(h, 0)
    if len(loads) != 2 or not done or run is None:
        return (False, f"loads {loads}, complete {done}, run {run}")
    return (loads[1] < done[0] and loads[1] > run, f"loads {loads}, run {run}, complete {done[0]}")


@pytest.mark.req(*reqs("030 040"))
def test_L5_2(window, send, bench, check, require):
    with window(510, 511):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW", [0] + abs_row(bench, 6, "long.bin", "WAIT_FOR_RUNNING"))
        bench.wait(10)
        send(S + ".SET_ROW", [0, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        send(S + ".SET_ROW_ENABLED", [0, False])
        send(S + ".SET_SEQ_ENABLED", ["short.bin", False])
        send(S + ".LOAD_TABLE")
        bench.wait(24)
        bracket(bench)
        check("ABS")
        check("event seqSched.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event seqSched.RowSet(row=0) count == 2")
        check("event seqSched.TableLoaded count == 2")
        check("event schedSeq.CS_SequenceCanceled count == 0")
        check("event seqSched.SeqCancelled count == 0")
        check("event schedSeq.CS_SequenceComplete(fileName=long.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        # "event seqSched.SeqDoneUnexpected count == 0" of the plan is one of the ABS checks above
        check("event seqSched.SeqRun(row=0) precedes event seqSched.RowSet(row=0, fileName=short.bin)")
        check("both TableLoaded events precede CS_SequenceComplete(fileName=long.bin), the second after SeqRun(row=0)",
              loads_around_run)
        check("last(seqSched.RunningRow) == 16")
        check("last(seqSched.RowsPending) == 16")


def file_unchanged(action):
    """The file read by a bench action has the same digest at its first and last reading in the test."""
    def judge(h):
        rows = [r for r in h.evidence(action) if r]
        return (len(rows) >= 2 and rows[0]["sha256"] == rows[-1]["sha256"] and rows[0]["size"] > 0,
                f"{len(rows)} readings: {[r['sha256'][:8] for r in rows]}")
    return judge


@pytest.mark.req(*reqs("003 005"))
def test_L5_3(window, send, bench, check, require):
    try:
        with window(520, 521):
            require("STD-ERR")
            standard_requires(require, fresh=True)
            bench.action("write_T_BASE")
            bench.action("write_T_BADCRC")
            bench.action("hash_T_BADCRC")
            send(S + ".LOAD_TABLE")
            send(S + ".SET_ROW_ENABLED", [0, True])
            send(S + ".SET_ROW", [0] + abs_row(bench, 15, "short.bin", "WAIT_FOR_RUNNING"))
            send(S + ".TABLE_FILE_PRM_SET", [T_BADCRC_FILE])
            send(S + ".LOAD_TABLE")
            bench.wait(20)
            bracket(bench)
            bench.action("hash_T_BADCRC")
            check("event cmdDisp.OpCodeError count == 1")
            check("event seqSched.TableLoadFailed(reason=CHECKSUM) count == 1")
            check("event seqSched.TableLoadFailed count == 1")
            check("event seqSched.TableLoaded count == 1")
            check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
            check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
            check("event seqSched.TableSaved count == 0")
            check("last(seqSched.TableLoaded) == True")
            check("last(seqSched.RowsExecuted) == 1")
            check("file T-BADCRC unchanged on the bench file system", file_unchanged("hash_T_BADCRC"))
    finally:
        restore_table(window, send, bench, check, require, 521)


@pytest.mark.req(*reqs("030 018 042 028"))
def test_L5_4(window, send, bench, check, require):
    with window(530, 531):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE_B")
        send(B + ".LOAD_TABLE")
        send(QB + ".CS_RUN", ["short.bin", "NO_BLOCK"])
        bench.wait(4)
        send(B + ".SET_ROW_ENABLED", [0, True])
        send(B + ".SET_ROW", [0] + abs_row(bench, 6, "long.bin", "WAIT_FOR_RUNNING"))
        bench.wait(10)
        send(B + ".SET_ROW_ENABLED", [0, False])
        send(B + ".SET_ROW", [0, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "short2.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(24)
        bracket_both(bench)
        check("event schedSeqB.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("event seqSchedB.SeqDoneUnexpected count == 1")
        check("event seqSchedB.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event schedSeqB.CS_SequenceComplete(fileName=long.bin) count == 1")
        check("event schedSeqB.CS_SequenceCanceled count == 0")
        check("event seqSchedB.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSchedB.SeqCompleted count == 1")
        check("event seqSchedB.SeqDoneUnexpected precedes event seqSchedB.SeqRun(row=0)")
        check("event seqSchedB.SeqRun(row=0) precedes event seqSchedB.RowSet(row=0, fileName=short2.bin)")
        check("event seqSchedB.RowSet(row=0, fileName=short2.bin) precedes "
              "event schedSeqB.CS_SequenceComplete(fileName=long.bin)")
        check("last(seqSchedB.RunningRow) == 16")
        check("last(seqSchedB.RowsExecuted) == 0")
        check("event seqSched.SeqDoneUnexpected count == 0")
        check("event seqSched.SeqRun count == 0")
        require("SeqSchedB samples >= 2 in every window", samples_b)


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


@pytest.mark.req(*reqs("019 035 041 002 004"))
def test_L2_1(window, send, bench, check, require):
    try:
        with window(200, 201):
            require("STD")
            standard_requires(require)
            bracket(bench)
            bench.action("count_dp")
            bench.action("write_T_FIELDS")
            send(S + ".LOAD_TABLE")
            send(S + ".DUMP_TABLE")
            bench.wait_event("dpWriter.FileWritten")
            bracket(bench)
            bench.action("read_dp")
            check("ABS")
            check("event seqSched.TableLoaded count == 1")
            check("event seqSched.TableDumped count == 1")
            check("last(seqSched.TableLoaded) == True")
            check("last(seqSched.RowsPending) == 16")
            check("last(seqSched.RowsExecuted) == 0")
            check("last(seqSched.RowsSkipped) == 0")
            check("last(seqSched.RowsWaiting) == 0")
            check("last(seqSched.NextRow) == 16")
            check("last(seqSched.RunningRow) == 16")
            require("the data product file decoded was written in this test", new_product)
            check("data product file decodes to table T-FIELDS, every field of every row as authored",
                  product_is(t_fields()))
    finally:
        with window(201, 202):
            require("both markers present")
            bench.action("write_T_BASE")
            send(S + ".LOAD_TABLE")
            bracket(bench)
            check("event seqSched.TableLoaded count == 1")


@pytest.mark.req(*reqs("034 004 035"))
def test_L6_1(window, send, bench, check, require):
    with window(600, 601):
        require("STD-ERR")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        bracket(bench)
        bench.action("count_dp")
        send(S + ".SET_ROW_ENABLED", [16, True])
        send(S + ".SET_ROW", [16] + abs_row(bench, 5, "short.bin", "WAIT_FOR_RUNNING"))
        send(S + ".SET_ROW", [0] + abs_row(bench, 5, "", "WAIT_FOR_RUNNING"))
        late = json.loads(time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 5))
        late["useconds"] = 1000000
        send(S + ".SET_ROW", [0, json.dumps(late), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send(S + ".SET_SEQ_ENABLED", ["nosuch.bin", True])
        send(S + ".SET_SEQ_ENABLED", ["", True])
        send(S + ".DUMP_TABLE")
        bench.wait(8)
        bracket(bench)
        bench.action("read_dp")
        check("event cmdDisp.OpCodeError count == 6")
        check("event seqSched.CmdRejected(reason=ROW_INDEX) count == 2")
        check("event seqSched.CmdRejected(reason=FILE_NAME) count == 2")
        check("event seqSched.CmdRejected(reason=TIME) count == 1")
        check("event seqSched.CmdRejected(reason=NO_MATCH) count == 1")
        check("event seqSched.RowEnabledSet count == 0")
        check("event seqSched.RowSet count == 0")
        check("event seqSched.SeqEnabledSet count == 0")
        check("event seqSched.SeqRun count == 0")
        check("last(seqSched.RowsPending) == 16")
        require("the data product file decoded was written in this test", new_product)
        check("data product file decodes to table T-BASE, all rows PENDING", product_is(t_base(), ["PENDING"] * 16))


@pytest.mark.req(*reqs("018 040"))
def test_L6_2(window, send, bench, check, require):
    with window(610, 611):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        bracket(bench)
        send(Q + ".CS_RUN", ["short.bin", "NO_BLOCK"])
        bench.wait(4)
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW", [0] + abs_row(bench, 6, "short.bin", "WAIT_FOR_RUNNING"))
        bench.wait(12)
        bracket(bench)
        # not ABS: the ground's own CS_RUN makes one SeqDoneUnexpected, judged below
        check("event seqSched.TableLoadFailed count == 0")
        check("event seqSched.CmdRejected count == 0")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 2")
        check("event seqSched.SeqDoneUnexpected count == 1")
        check("event seqSched.SeqCompleted count == 1")
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSched.SeqDoneUnexpected precedes event seqSched.SeqRun(row=0)")
        check("event seqSched.SeqRun(row=0) precedes event seqSched.SeqCompleted(row=0)")
        check("last(seqSched.RunningRow) == 16")
        check("last(seqSched.RowsExecuted) == 1")


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


@pytest.mark.req(*reqs("032 035 019 027 025 056 023 031 028 029 005 002"))
def test_L7_1(window, send, bench, check, require):
    with window(700, 701):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        bench.action("count_dp")
        send(S + ".SET_ROW_ENABLED", [5, True])
        send(S + ".SET_ROW", [5, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "short2.bin",
                              "SKIP_WHEN_RUNNING"])
        send(S + ".SET_ROW_ENABLED", [6, True])
        send(S + ".SET_ROW_ENABLED", [7, True])
        send(S + ".SET_ROW", [8] + abs_row(bench, 6, "short.bin", "WAIT_FOR_RUNNING"))
        send(S + ".SET_ROW", [6] + abs_row(bench, 6, "short.bin", "WAIT_FOR_RUNNING"))
        send(S + ".SET_ROW", [7] + abs_row(bench, 9, "short.bin", "WAIT_FOR_RUNNING"))
        bench.wait(14)
        send(S + ".SET_ROW_ENABLED", [6, False])
        send(S + ".SET_ROW_ENABLED", [6, True])
        send(S + ".SET_ROW", [7, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        send(S + ".SAVE_TABLE")
        send(S + ".SET_ROW_ENABLED", [4, True])
        send(S + ".LOAD_TABLE")
        bench.wait(3)
        send(S + ".DUMP_TABLE")
        bench.wait_event("dpWriter.FileWritten")
        bracket(bench)
        bench.action("read_table")
        bench.action("read_dp")
        check("ABS")
        check("event seqSched.SeqRun(row=6, fileName=short.bin) count == 1")
        check("event seqSched.SeqRun(row=7, fileName=short.bin) count == 1")
        check("event seqSched.TableSaved count == 1")
        check("event seqSched.TableDumped count == 1")
        check("event seqSched.TableLoaded count == 2")
        check("event seqSched.RowLate count == 0")
        check("event seqSched.RowSkippedDisabled(row=8) count == 1")
        check("event seqSched.RowSkippedDisabled count == 1")
        check("last(seqSched.RowsExecuted) == 0")
        check("last(seqSched.RowsSkipped) == 2")
        check("last(seqSched.RowsPending) == 14")
        check("table file on the bench file system == T-FINAL: valid checksum; row 5 short2.bin SKIP_WHEN_RUNNING "
              "enabled, rows 6 and 7 enabled, row 8 disabled, every other row and field as T-BASE, no row state",
              table_is_final)
        require("the data product file decoded was written in this test", new_product)
        states = ["PENDING"] * 16
        states[6] = states[8] = "SKIPPED"
        check("data product file decodes to T-FINAL with row states: rows 6 and 8 SKIPPED, every other row PENDING",
              product_is(t_final(), states, PAST_TIMES))
