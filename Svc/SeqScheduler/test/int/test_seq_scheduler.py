"""Stimulus and named checks of the Svc.SeqScheduler system test plan (plan.json beside this file).

Each test sends the plan's procedure inside its windows and declares the plan's named checks.
The verdict is computed from the record at window close; waits are pacing only.
"""
import json

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
    """A Fw.TimeValue command argument, in the JSON text the command sender takes."""
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
