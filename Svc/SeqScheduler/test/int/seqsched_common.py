"""Stimulus and named checks of the Svc.SeqScheduler system test plan (plan.json beside this file).

Each test sends the plan's procedure inside its windows and declares the plan's named checks.
The verdict is computed from the record at window close; waits are pacing only.
"""
import ast
import json
import os
import re
from pathlib import Path

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
FRESH = "each 'FSW time + N s' is taken from the newest recorded event, logged <= 1 s before the reading"
FRESH4 = "each 'FSW time + N s' is taken from the newest recorded event, logged <= 4 s before the reading"


def reqs(numbers):
    """Requirement ids of the component from their numbers."""
    return ["REQ-SeqScheduler-" + n for n in numbers.split()]


def time_value(base, seconds):
    """A Fw.TimeValue command argument, in the JSON text a command takes on this bench."""
    return json.dumps({"timeBase": base, "timeContext": 0, "seconds": int(seconds), "useconds": 0})


# Both hold for the life of the process, which is one run (`fth run` starts one pytest process); a second run of
# a test in the same process would find the readings of the first.
_READINGS = []  # the test of each FSW time reading of this process, in the order taken
_JUDGED = {}  # per test, the number of its readings that a window has judged


def fsw_now(bench):
    """Seconds of the newest recorded event (0 in a dry run, where nothing is recorded)."""
    rows = bench.action("fsw_now")
    if bench.dry_run and bench.replay_record is None:
        return 0  # a dry run records nothing; a replay is also a dry run of the bench, but it has the readings
    if not rows:
        raise RuntimeError("bench: no FSW time reading (fsw_now printed no event)")
    _READINGS.append(bench.test_id)  # one evidence row per reading, also for one that failed
    if not rows[-1]:
        raise RuntimeError("bench: no FSW time reading (fsw_now printed no event)")
    return int(rows[-1]["sec"])


def fresh_times(limit=1.0):
    """The FSW times read in this window are at most `limit` seconds old.

    Built when the window opens and judged when it closes: the readings of the window are those taken between
    the two, by their position among the readings of the test. A live run and a replay take the readings in
    the same order, so both judge the same rows; the time of a reading plays no part. Every window that reads
    an FSW time states this rule. A window with no reading of its own fails, and so does one that finds
    readings of the test before it that no window judged; `fsw_now` raises on a reading that failed.
    """
    start = len(_READINGS)

    def judge(h):
        end = len(_READINGS)
        if end <= start:
            return (False, "no FSW time reading in the window")
        test, base = _READINGS[start], start
        while base > 0 and _READINGS[base - 1] == test:
            base -= 1
        rows = h.evidence("fsw_now")
        mine = rows[start - base:end - base]
        unjudged = (start - base) - _JUDGED.get(test, 0)
        _JUDGED[test] = max(_JUDGED.get(test, 0), end - base)
        ages = [r["age_s"] for r in mine if r]
        ok = len(mine) == end - start and len(ages) == len(mine) and max(ages, default=limit + 1) <= limit
        return (ok and unjudged <= 0,
                f"ages {ages} of readings {start - base + 1}..{end - base} of {len(rows)} in the test; "
                f"{max(unjudged, 0)} earlier readings judged by no window")
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


def _raw_args(raw):
    """The recorded argument list of a command, each argument as text; None when it is not a list."""
    try:
        args = ast.literal_eval(raw.strip())
    except (ValueError, SyntaxError):
        return None
    return [str(a) for a in args] if isinstance(args, list) else None


def plan_params(h):
    """No parameter set command of the record before this window left a plan parameter off its plan value.

    The image starts with the plan values (no parameter is saved), so the last set command before the window,
    if any, decides; its recorded argument list is compared whole, not searched."""
    last = {}
    for c in h.record.commands:
        if c.short in PLAN_PARAMS and c.idx < h.window.cmds[0].idx:
            last[c.short] = c
    wrong = [f"{k}={c.raw}" for k, c in last.items() if _raw_args(c.raw) != [str(PLAN_PARAMS[k])]]
    return (not wrong, f"{len(last)} plan parameters set before the window; off plan: {wrong}")


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


SET_AHEAD = "every ABSOLUTE row set for a coming time was dispatched before that time"


def rows_set_ahead(h):
    """Every SET_ROW of the window that carries an ABSOLUTE time in TB_WORKSTATION_TIME before the year 2100 was
    dispatched before that time: a row command dispatched only at or after its row time (a slow command) makes
    the row late by the bench's doing, not the unit's. Judged on the command sent and the dispatcher's event, not
    on what the unit reports; a command of that kind without its dispatch event fails the rule."""
    late, judged, wanted = [], 0, 0
    for short in ("seqSched.SET_ROW", "seqSchedB.SET_ROW"):
        rows = [c for c in h.window.cmds if c.short == short]
        if not rows:
            continue
        sent = [e for e in h.window.events("cmdDisp.OpCodeDispatched")
                if [int(code, 16) for code in re.findall(r"0x([0-9a-f]+)", e.text.lower())][:1] == [rows[0].opcode]]
        for nth, c in enumerate(rows):
            try:
                args = ast.literal_eval(c.raw.strip())
            except (ValueError, SyntaxError):
                args = None
            if not isinstance(args, list) or len(args) < 3 or not isinstance(args[1], dict):
                wanted += 1  # not readable: cannot be shown to be set ahead
                continue
            time = args[1]
            if (str(args[2]).split(".")[-1] != "ABSOLUTE" or str(time.get("timeBase")).split(".")[-1] !=
                    "TB_WORKSTATION_TIME" or int(time.get("seconds", YEAR_2100)) >= YEAR_2100):
                continue
            wanted += 1
            if nth >= len(sent):
                continue
            judged += 1
            if sent[nth].fsw >= int(time["seconds"]):
                late.append(f"{short} row {args[0]} dispatched at {sent[nth].fsw:.3f} for {time['seconds']}")
    return (not late and judged == wanted,
            f"{judged} of {wanted} rows for a coming time judged; dispatched late: {late or 'none'}")


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
    sent = [e for e in w.events("cmdDisp.OpCodeDispatched")
            if [int(code, 16) for code in re.findall(r"0x([0-9a-f]+)", e.text.lower())][:1] == [opcode]]
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


def manifest_in_hand():
    """Describe strings of the manifest given to this `fth run` or replay (named in FTH_MANIFEST), or None."""
    path = os.environ.get("FTH_MANIFEST")
    if not path or not Path(path).is_file():
        return None
    repos = json.loads(Path(path).read_text(encoding="utf-8")).get("repos", [])
    return sorted({str(r["describe"]) for r in repos if r.get("describe")})


def version_strings_match(h):
    """Every describe string of the manifest in hand is, whole, the version a version event of the window names.

    The strings the bench read from the manifest in the live run are in the record; a replay compares them with
    the manifest it was given, so a record judged against another manifest fails."""
    rows = [r for r in h.evidence("manifest_versions") if r]
    recorded = sorted(rows[0].get("describe", [])) if len(rows) == 1 else []
    wanted = manifest_in_hand()
    named = [found.group(1) for e in h.window.ev if ".version." in e.name
             for found in [re.search(r"\[([^\]]*)\]\s*$", e.text)] if found]
    if not recorded or wanted is None or not named:
        return (False, f"recorded manifest strings {recorded}, manifest in hand {wanted}, {len(named)} version events")
    missing = [d for d in wanted if d not in named]
    return (recorded == wanted and not missing,
            f"manifest in hand {wanted}, recorded {recorded}, version events name {sorted(set(named))}")






FIVE = ("cmdDisp", "health", "seqSched", "schedSeq", "systemResources")
SPAN_RECORDS = ("at least one record from each of CdhCore.cmdDisp, CdhCore.health, Ref.seqSched, Ref.schedSeq and "
                "Ref.systemResources lies inside the override span, 1 s clear of both ends")
SPAN_BASE = ("every event and channel record of CdhCore.cmdDisp, CdhCore.health, Ref.seqSched, Ref.schedSeq and "
             "Ref.systemResources time-tagged more than 1 s after the first TimeBaseSet and more than 1 s before "
             "TimeBaseCleared carries TB_SC_TIME, and no record of the window more than 1 s outside that span does")
ACTIVE_ROW = "the {}.ActiveRow event carries the time of row 1's SET_ROW command and its file name short.bin"
ONE_CLOCK = "bench and FSW share one clock: the FSW seconds of both marker events are within 2 s of their ground receive time"
SPAN_STAMPED = "every event of the override span has a ground receive time"
RUNNING_AT_ENABLES = "long.bin is running on Ref.schedSeq when each SET_ENABLED command of the window is handled"
SPAN_RECEIVE = "FSW seconds of every event in the override span are within 2 s of its ground receive time"
SPAN_SECONDS = ("FSW seconds of every record in the override span lie between those of the last record before "
                "the span and the first record after it")


RECEIVED = "every event of the window was received within 10 s of its FSW time tag"
RECEIVE_LAG_S = 10.0


def received_in_time(h):
    """A bench condition: the bench waits on the event log, so events logged long after they were emitted make
    its waits time out and its commands late by the recorder's doing. An event without a receive time fails.
    Member of STD and STD-ERR as the helper `events_received_in_time`; required by name in windows without them."""
    rows = [h.window.start_marker, h.window.end_marker] + list(h.window.ev)
    unstamped = [e.line for e in rows if e.recv is None]
    late = [e.line for e in rows if e.recv is not None and e.recv - e.fsw > RECEIVE_LAG_S]
    worst = max((e.recv - e.fsw for e in rows if e.recv is not None), default=0.0)
    return (not unstamped and not late,
            f"{len(rows)} events, {len(unstamped)} without a receive time, {len(late)} over {RECEIVE_LAG_S:g} s, "
            f"largest {worst:.3f} s")


def one_clock(h):
    """A bench condition: several helpers compare a bench wall-clock time with an FSW time directly."""
    marks = [h.window.start_marker, h.window.end_marker]
    apart = [None if m.recv is None else abs(m.fsw - m.recv) for m in marks]
    return (all(a is not None and a <= 2.0 for a in apart), f"FSW time minus receive time of the markers: {apart}")


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


def span_events(h):
    """The events of the override span: those whose time tag carries the override's base (span_base judges that)."""
    return [e for e in h.window.ev if e.base == TB["TB_SC_TIME"]]


def span_stamped(h):
    """A bench condition: an event the recorder did not stamp cannot be judged by span_receive."""
    inside = span_events(h)
    unstamped = [e.line for e in inside if e.recv is None]
    return (bool(inside) and not unstamped, f"{len(inside)} events, {len(unstamped)} without a receive time")


def span_receive(h):
    """The approved L0-2 rule: the override changes the time base of a time tag, not its seconds."""
    inside = [e for e in span_events(h) if e.recv is not None]
    off = [e.line for e in inside if abs(e.fsw - e.recv) > 2.0]
    worst = max((abs(e.fsw - e.recv) for e in inside), default=0.0)
    return (bool(inside) and not off, f"{len(inside)} events, {len(off)} over 2 s, largest {worst:.3f} s")


def enables_while_running(row):
    """A bench condition of L2-17: every SET_ENABLED was handled between the run of `row` and its completion."""
    def judge(h):
        start = row_event_time(h, "seqSched.SeqRun", row)
        end = row_event_time(h, "seqSched.SeqCompleted", row)
        sets = [e.fsw for e in h.window.events("seqSched.EnabledSet")]
        if start is None or end is None or not sets:
            return (False, f"run {start}, completion {end}, {len(sets)} EnabledSet")
        outside = [t for t in sets if not start < t < end]
        return (not outside, f"{len(sets)} EnabledSet, {len(outside)} outside {start:.3f}..{end:.3f}")
    return judge


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
    """Row 1 is set in W1 and run in W2, judged against the row time the command of W1 carries: the time at
    which the command was handled plays no part (the FSW time reading it is built from is seconds older)."""
    run = run_time(h, 1, "W2")
    sent = [c for c in h.windows["W1"].cmds if c.short == "seqSched.SET_ROW" and c.args[:1] == [1]]
    if run is None or not sent or len(sent[0].args) < 3:
        return (False, f"run {run}, SET_ROW commands of row 1: {len(sent)}")
    late = run - sent[0].args[2]
    return (0 <= late <= 3, f"{late:.3f} s after row time {sent[0].args[2]}")


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


def active_row_is(instance, row, file_name):
    """The ActiveRow report of `row` names the time its last SET_ROW of the window carries, and its file."""
    def judge(h):
        reports = [e for e in h.window.events(f"{instance}.ActiveRow") if e.args.get("row") == row]
        sent = [c for c in h.window.cmds if c.short == f"{instance}.SET_ROW" and c.args[:1] == [row]]
        found = re.search(r"'seconds': (\d+)", str(reports[0].args.get("timeDue"))) if reports else None
        if found is None or not sent or len(sent[-1].args) < 3:
            return (False, f"{len(reports)} reports, {len(sent)} SET_ROW commands")
        return (int(found.group(1)) == sent[-1].args[2] and reports[0].args.get("fileName") == file_name,
                f"reported {found.group(1)} {reports[0].args.get('fileName')}, commanded {sent[-1].args[2]}")
    return judge


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
        # The two channels of one tick are written one after the other: their times differ by microseconds
        hits = [t for t in wait
                if wait[t] == waiting and any(abs(u - t) < 0.5 and running[u] == row for u in running)]
        return (bool(hits), f"{len(wait)} samples in the span, {len(hits)} matching")
    return judge




def abs_time(bench, offset):
    """SET_ROW time of an ABSOLUTE row due `offset` s after the newest recorded event."""
    return time_value("TB_WORKSTATION_TIME", fsw_now(bench) + offset)


def row_time(base, offset):
    """SET_ROW time of an ABSOLUTE row due `offset` s after `base`, one FSW time reading shared by the rows of a
    group: rows set from one reading keep their spacing whatever time the commands take."""
    return time_value("TB_WORKSTATION_TIME", base + offset)


YEAR_2100 = 4102444800



def samples_b(h):
    """The helper's own window only: a window the live run has not reached yet holds no sample."""
    n = len(h.window.channel(SAMPLE_B))
    return (n >= 2, f"SeqSchedB samples={n}")






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
        return (low <= run - start[1]["start"] <= high, f"{run - start[1]['start']:.3f} s")
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
        return (len(rows) == 2 and rows[0]["sha256"] == rows[1]["sha256"] and rows[0]["size"] > 0,
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
        if len(rows) != 1 or rows[0]["rows"] is None:
            return (False, f"no decoded product: {rows[0]['error'] if len(rows) == 1 else str(len(rows)) + ' read_dp rows, one wanted'}")
        got = rows[0]["rows"]
        wrong = differing(got, want, FIELDS, skip)
        if states is not None and not wrong:
            wrong = [f"row {i} state: {got[i].get('state')!r} != {states[i]!r}" for i in range(len(want))
                     if got[i].get("state") != states[i]]
        return (not wrong, f"{rows[0]['file']}: {wrong[:4] if wrong else 'as wanted'}")
    return judge


def new_product(h):
    """The product decoded was written in this test: more product files after the dump than before it."""
    before = [r for r in h.evidence("count_dp") if r]
    after = [r for r in h.evidence("read_dp") if r]
    return (len(before) == 1 and len(after) == 1 and after[0]["files"] > before[0]["files"],
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
    if len(rows) != 1 or not rows[0]["valid"]:
        return (False, f"table file not valid: {rows[0]['error'] if len(rows) == 1 else str(len(rows)) + ' read_table rows, one wanted'}")
    got = rows[0]["rows"]
    wrong = differing(got, t_final(), FIELDS, PAST_TIMES)
    if not wrong:
        wrong = [f"row {i} carries a state" for i in range(16) if "state" in got[i]]
    if not wrong:
        wrong = [f"row {i} time {got[i]['seconds']} is not a past FSW time" for i, _ in PAST_TIMES
                 if not 0 < got[i]["seconds"] < h.window.end_marker.fsw]
    return (not wrong, f"{wrong[:4] if wrong else 'as wanted'}")


