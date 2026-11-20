"""Known-answer tests of the helper predicates of the SeqScheduler system tests.

Each helper of ../int/seqsched_common.py and ../int/planted_faults.py is given a small hand-made
record on which it must hold and one on which it must not. The rows are hand-made; the record, its
windows and the helper context are the tool's own (fth.replay.Record and Window, fth.engine.HelperContext),
so a helper is judged on the rows the tool would hand it. Run: `pytest Svc/SeqScheduler/test/kat`.
"""
import json
import sys
from pathlib import Path
from types import SimpleNamespace

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "int"))

import seqsched_common as sc  # noqa: E402
from fth.registry import load_helpers_module  # noqa: E402
from fth.engine import HelperContext  # noqa: E402
from fth.replay import ChannelRow, CommandRow, EventRow, Record, Window  # noqa: E402

HELPERS = load_helpers_module(Path(__file__).resolve().parent.parent / "int" / "planted_faults.py").helpers
NO_OP_OPCODE = 0x1000000  # CdhCore.cmdDisp.CMD_NO_OP
MARK_OPCODE = 0x1000001  # CdhCore.cmdDisp.CMD_NO_OP_STRING

T0 = 1000.0  # FSW time of the opening marker
WS, SC_TIME = 2, 3


class Rec:
    """Builds a record of one start (epoch 0) and helper contexts over it."""

    def __init__(self):
        self.events, self.channels, self.commands, self.evidence = [], [], [], {}

    def ev(self, short, fsw, text="", base=WS, epoch=0, **args):
        row = EventRow(len(self.events) + 1, fsw, epoch, fsw, "Ref." + short, short, 0, "ACTIVITY_HI", text, args, {},
                       base)
        self.events.append(row)
        return row

    def ch(self, short, fsw, value, base=WS, epoch=0):
        row = ChannelRow(len(self.channels) + 1, fsw, epoch, fsw, "Ref." + short, short, 0, value, value, base)
        self.channels.append(row)
        return row

    def cmd(self, short, opcode, args=(), raw=""):
        row = CommandRow(len(self.commands) + 1, "", "Ref." + short, short, opcode, list(args), raw, "OpCodeCompleted")
        self.commands.append(row)
        return row

    def mark(self, marker, fsw):
        self.cmd("cmdDisp.CMD_NO_OP_STRING", MARK_OPCODE, raw=f"['{marker}']")
        return self.ev("cmdDisp.NoOpStringReceived", fsw, id=marker)

    def record(self):
        return Record(Path("kat"), self.events, self.channels, self.commands,
                      marker_event="cmdDisp.NoOpStringReceived", marker_command="cmdDisp.CMD_NO_OP_STRING",
                      evidence=[{"test": "KAT", "action": action, "data": row}
                                for action, rows in self.evidence.items() for row in rows])

    def win(self, start, end):
        """The fth.replay.Window between two marker events, sliced by the tool as it slices a record."""
        w = Window(self.record(), start.args["id"], end.args["id"])
        # on a record whose dispatcher logs the dispatch first, the tool opens the window on the marker's dispatch row
        assert w.ok and w.end_marker is end and start in w.ev[:2]
        return w

    def ctx(self, window, **windows):
        record = self.record()
        test = SimpleNamespace(id="KAT")
        return HelperContext(record, None, test, window, dict(windows) or {"W": window})


def simple(build=None, evidence=None):
    """A window T0 .. T0+60 with what `build` adds between the markers."""
    rec = Rec()
    start = rec.mark(100, T0)
    if build:
        build(rec)
    end = rec.mark(101, T0 + 60)
    rec.evidence.update(evidence or {})
    return rec.ctx(rec.win(start, end))


def holds(result):
    return bool(result[0])


def row_set(rec, row, seconds, fsw):
    rec.ev("seqSched.RowSet", fsw, f"Row {row} set: {{'timeBase': 'TB_WORKSTATION_TIME', 'seconds': {seconds}}}", row=row)


def test_fresh_times():
    class Bench:
        dry_run, replay_record = False, None

        def __init__(self, test_id):
            self.test_id, self.rows = test_id, []

        def action(self, name):
            return self.rows[-1:]

    def window(bench, ages, limit=1.0, before=()):
        """Readings `before` the window opens, then one window that takes a reading of each age in `ages`."""
        def take(age):
            bench.rows.append(None if age is None else {"age_s": age, "sec": 7, "usec": 0})
            try:
                sc.fsw_now(bench)
            except RuntimeError:
                pass
        for age in before:
            take(age)
        judge = sc.fresh_times(limit)
        for age in ages:
            take(age)
        return judge(simple(evidence={"fsw_now": list(bench.rows)}))

    assert holds(window(Bench("T-1"), [0.4, 1.0]))
    assert not holds(window(Bench("T-2"), [0.4, 1.5]))
    assert not holds(window(Bench("T-3"), []))  # no reading of its own
    assert not holds(window(Bench("T-4"), [0.4, None]))  # a failed reading
    assert holds(window(Bench("T-5"), [1.6, 4.0], limit=4.0))
    assert not holds(window(Bench("T-5b"), [1.6, 4.1], limit=4.0))
    # two windows of one test: each judges its own readings, by position, whatever their times
    two = Bench("T-6")
    assert holds(window(two, [0.2, 0.3]))
    assert not holds(window(two, [1.5]))
    assert holds(window(two, [0.9]))
    # another test's stale readings earlier in the process are not this test's
    assert holds(window(Bench("T-7"), [0.5]))
    # a reading of the test taken before the window opened, which no window judged
    assert not holds(window(Bench("T-8"), [0.5], before=[0.1]))


def test_fsw_now_raises_on_a_failed_reading():
    class Bench:
        test_id = "T-0"

        def __init__(self, rows, dry_run=False, replay_record=None):
            self.rows, self.dry_run, self.replay_record = rows, dry_run, replay_record

        def action(self, name):
            return self.rows

    assert sc.fsw_now(Bench([{"sec": 7, "usec": 0, "age_s": 0.1}])) == 7
    assert sc.fsw_now(Bench([], dry_run=True)) == 0
    # A replay is a dry run of the bench that has the recorded readings: they are used, and a failed one raises
    assert sc.fsw_now(Bench([{"sec": 7, "usec": 0, "age_s": 0.1}], dry_run=True, replay_record="record")) == 7
    with pytest.raises(RuntimeError):
        sc.fsw_now(Bench([None], dry_run=True, replay_record="record"))
    for rows in ([], [None], [{"sec": 7}, None]):
        with pytest.raises(RuntimeError):
            sc.fsw_now(Bench(rows))


def test_sequencers_idle():
    def before(ended):
        rec = Rec()
        rec.ev("schedSeq.CS_SequenceLoaded", T0 - 20)
        if ended:
            rec.ev("schedSeq.CS_SequenceComplete", T0 - 10)
        start = rec.mark(100, T0)
        return rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))
    assert holds(sc.sequencers_idle(before(True)))
    assert not holds(sc.sequencers_idle(before(False)))


def test_plan_params():
    def with_set(raw):
        rec = Rec()
        rec.cmd("seqSched.LATE_TOLERANCE_SEC_PRM_SET", 5, raw=raw)
        start = rec.mark(100, T0)
        rec.cmd("cmdDisp.CMD_NO_OP_STRING", 1, [100])
        ctx = rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))
        ctx.window.cmds = rec.commands[1:]
        return ctx
    assert holds(sc.plan_params(with_set("['1']")))
    assert holds(sc.plan_params(with_set("[1]")))
    assert not holds(sc.plan_params(with_set("['5']")))
    assert not holds(sc.plan_params(with_set("['10']")))  # holds the plan value as a substring only
    assert not holds(sc.plan_params(with_set("['1', '1']")))
    assert not holds(sc.plan_params(with_set("not a list")))
    assert holds(sc.plan_params(simple()))  # no parameter command before the window: the start-up values stand


def test_run_not_before_row_time():
    def case(run, commanded=1010, reported=1010):
        def build(rec):
            rec.cmd("seqSched.SET_ROW", 7, [0, 0, commanded, 0])
            row_set(rec, 0, reported, T0 + 1)
            rec.ev("seqSched.SeqRun", run, row=0)
        return simple(build)
    assert holds(sc.run_not_before_row_time(0)(case(1010.2)))
    assert not holds(sc.run_not_before_row_time(0)(case(1009.2)))  # run early
    assert not holds(sc.run_not_before_row_time(0)(case(1010.2, reported=1005)))  # RowSet reports another time
    assert not holds(sc.run_not_before_row_time(0)(case(1007.0, commanded=1010, reported=1005)))
    assert not holds(sc.run_not_before_row_time(1)(case(1010.2)))  # no such row


def test_run_not_before_delay():
    def case(run, start=({"start": 990.0},)):
        def build(rec):
            row_set(rec, 0, 30, T0 + 1)
            rec.ev("seqSched.SeqRun", run, row=0)
        return simple(build, {"ref_start": list(start)})
    assert holds(sc.run_not_before_delay(0)(case(1020.5)))
    assert not holds(sc.run_not_before_delay(0)(case(1019.5)))
    assert not holds(sc.run_not_before_delay(0)(case(1020.5, start=())))


def test_command_time_and_run_after():
    def case(run):
        def build(rec):
            rec.cmd("seqSched.SET_ROW", 0x10000a04)
            rec.ev("cmdDisp.OpCodeDispatched", T0 + 2, "Opcode 0x10000a04 dispatched to port 0")
            rec.ev("seqSched.SeqRun", run, row=0)
        return simple(build)
    assert sc.command_time(case(T0 + 10), "seqSched.SET_ROW") == T0 + 2
    assert sc.command_time(case(T0 + 10), "seqSched.SET_ROW", 1) is None

    def prefix(rec):  # an opcode that is the leading digits of another's is not that command
        rec.cmd("seqSched.SET_ROW", 0x10000a0)
        rec.ev("cmdDisp.OpCodeDispatched", T0 + 2, "Opcode 0x10000a04 dispatched to port 0")
    assert sc.command_time(simple(prefix), "seqSched.SET_ROW") is None
    assert holds(sc.run_after(0, "seqSched.SET_ROW", 8, 12)(case(T0 + 12)))
    assert not holds(sc.run_after(0, "seqSched.SET_ROW", 8, 12)(case(T0 + 9)))
    assert not holds(sc.run_after(0, "seqSched.SET_ROW", 8, 12)(case(T0 + 15)))
    assert not holds(sc.run_after(0, "seqSched.LOAD_TABLE", 8, 12)(case(T0 + 12)))


def test_version_strings_match(tmp_path, monkeypatch):
    def manifest(*describe):
        path = tmp_path / "manifest.json"
        path.write_text(json.dumps({"repos": [{"describe": d} for d in describe]}))
        monkeypatch.setenv("FTH_MANIFEST", str(path))

    def case(text, wanted=({"describe": ["608087211"]},)):
        return simple(lambda rec: rec.ev("version.FrameworkVersion", T0 + 1, text),
                      {"manifest_versions": list(wanted)})
    manifest("608087211")
    assert holds(sc.version_strings_match(case("Framework Version: [608087211]")))
    assert not holds(sc.version_strings_match(case("Framework Version: [bogus]")))
    assert not holds(sc.version_strings_match(case("Framework Version: [v4.0.0-12-g608087211]")))  # not the whole string
    assert not holds(sc.version_strings_match(case("Framework Version: 608087211")))  # no version named
    assert not holds(sc.version_strings_match(case("Framework Version: [608087211]", wanted=[{"describe": []}])))
    assert not holds(sc.version_strings_match(case("Framework Version: [608087211]", wanted=[])))
    assert not holds(sc.version_strings_match(simple(evidence={"manifest_versions": [{"describe": ["608087211"]}]})))
    # the record judged against another manifest than the one of its live run
    manifest("736179b67")
    assert not holds(sc.version_strings_match(case("Framework Version: [608087211]")))
    assert not holds(sc.version_strings_match(case("Framework Version: [736179b67]")))
    # no manifest in hand
    monkeypatch.delenv("FTH_MANIFEST")
    assert not holds(sc.version_strings_match(case("Framework Version: [608087211]")))


def test_one_clock():
    def case(start, end):
        ctx = simple()
        ctx.window.start_marker.recv, ctx.window.end_marker.recv = start, end
        return ctx
    assert holds(sc.one_clock(case(T0 + 0.01, T0 + 62.0)))
    assert not holds(sc.one_clock(case(T0 + 0.01, T0 + 62.1)))
    assert not holds(sc.one_clock(case(T0 - 3600, T0 + 60 - 3600)))  # the bench an hour behind
    assert not holds(sc.one_clock(case(None, T0 + 60)))  # a marker without a receive time


def test_no_start_up():
    assert holds(sc.no_start_up(simple()))
    assert not holds(sc.no_start_up(simple(lambda rec: rec.ev("prmDb.PrmFileLoadComplete", T0 + 5))))


def span(inside_base=SC_TIME, outside_base=WS, missing=None, inside_fsw=T0 + 20):
    """Override span T0+10 .. T0+30 with one record of each of the five instances inside it."""
    def build(rec):
        rec.ch("seqSched.NextTimeDue", T0 + 5, 0, outside_base)
        rec.ev("timeBaseSetter.TimeBaseSet", T0 + 10, base=SC_TIME)
        for name in sc.FIVE:
            if name != missing:
                rec.ch(name + ".Sample", inside_fsw, 0, inside_base)
        rec.ev("timeBaseSetter.TimeBaseCleared", T0 + 30)
        rec.ch("seqSched.NextTimeDue", T0 + 35, 0, outside_base)
    return simple(build)


def test_span_receive():
    def case(offsets):
        def build(rec):
            rec.ev("seqSched.RowSet", T0 + 5).recv = T0 + 9.0  # outside the span: not judged
            for i, offset in enumerate(offsets):
                rec.ev("seqSched.RowSet", T0 + 20 + i, base=SC_TIME).recv = None if offset is None else T0 + 20 + i + offset
        return simple(build)
    assert holds(sc.span_receive(case([0.01, 1.9, -1.9])))
    assert holds(sc.span_receive(case([2.0])))  # at the limit
    assert not holds(sc.span_receive(case([0.01, 2.1])))  # seconds 2.1 s behind the receive time
    assert not holds(sc.span_receive(case([0.01, -2.1])))
    assert not holds(sc.span_receive(case([])))  # no event in the span
    assert not holds(sc.span_receive(case([None])))  # nothing to judge
    # an event the recorder did not stamp is a bench condition, judged by span_stamped
    assert holds(sc.span_receive(case([0.01, None])))
    assert holds(sc.span_stamped(case([0.01, 2.5])))
    assert not holds(sc.span_stamped(case([0.01, None])))
    assert not holds(sc.span_stamped(case([])))
    assert len(sc.span_events(case([0.01, None]))) == 2


def test_enables_while_running():
    def case(sets, run=T0 + 10, done=T0 + 40):
        def build(rec):
            if run is not None:
                rec.ev("seqSched.SeqRun", run, row=0)
            for fsw in sets:
                rec.ev("seqSched.EnabledSet", fsw)
            if done is not None:
                rec.ev("seqSched.SeqCompleted", done, row=0)
            rec.ev("seqSched.SeqRun", T0 + 5, row=3)  # another row's run is not the bound
        return simple(build)
    assert holds(sc.enables_while_running(0)(case([T0 + 20, T0 + 22])))
    assert holds(sc.enables_while_running(0)(case([T0 + 10.001, T0 + 39.999])))
    assert not holds(sc.enables_while_running(0)(case([T0 + 9, T0 + 22])))  # disabled before long.bin ran
    assert not holds(sc.enables_while_running(0)(case([T0 + 20, T0 + 41])))  # enabled after it completed
    assert not holds(sc.enables_while_running(0)(case([T0 + 20, T0 + 40])))  # at the completion: not inside
    assert not holds(sc.enables_while_running(0)(case([])))
    assert not holds(sc.enables_while_running(0)(case([T0 + 20], run=None)))
    assert not holds(sc.enables_while_running(0)(case([T0 + 20], done=None)))


def test_override_span_helpers():
    assert holds(sc.span_has_records(span()))
    assert not holds(sc.span_has_records(span(missing="health")))
    assert not holds(sc.span_has_records(simple()))
    assert holds(sc.span_base(span()))
    assert not holds(sc.span_base(span(inside_base=WS)))
    assert not holds(sc.span_base(span(outside_base=SC_TIME)))
    assert not holds(sc.span_base(simple()))
    assert holds(sc.span_seconds(span()))
    assert not holds(sc.span_seconds(span(inside_fsw=T0 + 50)))  # seconds altered: later than the records after
    assert not holds(sc.span_seconds(span(inside_fsw=T0 + 2)))  # earlier than the records before
    assert not holds(sc.span_seconds(simple()))
    # missing evidence: a span with no record of the other base before it, inside it, after it
    for gone in ("before", "inside", "after"):
        ctx = span()
        lo, hi = sc.override_span(ctx)
        keep = {"before": lambda r: not (r.fsw < lo and r.base != SC_TIME),
                "inside": lambda r: r.base != SC_TIME,
                "after": lambda r: not (r.fsw > hi and r.base != SC_TIME)}[gone]
        for r in ctx.window.ev:  # the two events that bound the span stay, in the other base
            if r.short.startswith("timeBaseSetter.") and gone == "inside":
                r.base = WS
        ctx.window.ev = [r for r in ctx.window.ev if keep(r) or r.short.startswith("timeBaseSetter.")]
        ctx.window.ch = [r for r in ctx.window.ch if keep(r)]
        assert not holds(sc.span_seconds(ctx)), gone


def test_second_set_base():
    def case(base):
        def build(rec):
            rec.ev("timeBaseSetter.TimeBaseSet", T0 + 10)
            rec.ev("timeBaseSetter.TimeBaseSet", T0 + 20, base=base)
        return simple(build)
    assert holds(sc.second_set_base(case(SC_TIME)))
    assert not holds(sc.second_set_base(case(WS)))
    assert not holds(sc.second_set_base(simple(lambda rec: rec.ev("timeBaseSetter.TimeBaseSet", T0 + 10, base=SC_TIME))))


def two_windows(build1=None, build2=None, evidence=None, names=("W1", "W2")):
    rec = Rec()
    s1 = rec.mark(100, T0)
    if build1:
        build1(rec)
    e1 = rec.mark(101, T0 + 40)
    s2 = rec.mark(102, T0 + 41)
    if build2:
        build2(rec)
    e2 = rec.mark(103, T0 + 100)
    rec.evidence.update(evidence or {})
    w1, w2 = rec.win(s1, e1), rec.win(s2, e2)
    return rec.ctx(w2, **{names[0]: w1, names[1]: w2})


def test_row1_after_its_set_row():
    def case(run):
        def build1(rec):
            rec.cmd("seqSched.SET_ROW", 0xa04)
            rec.cmd("seqSched.SET_ROW", 0xa04)
            rec.ev("cmdDisp.OpCodeDispatched", T0 + 2, "Opcode 0xa04 dispatched to port 0")
            rec.ev("cmdDisp.OpCodeDispatched", T0 + 4, "Opcode 0xa04 dispatched to port 0")
        return two_windows(build1, lambda rec: rec.ev("seqSched.SeqRun", run, row=1))
    assert holds(sc.row1_after_its_set_row(case(T0 + 34)))
    assert not holds(sc.row1_after_its_set_row(case(T0 + 42)))
    assert holds(sc.row1_after_its_set_row(case(T0 + 31)))  # 27 s after the second SET_ROW: the lower limit
    assert holds(sc.row1_after_its_set_row(case(T0 + 37)))  # 33 s: the upper limit
    assert not holds(sc.row1_after_its_set_row(case(T0 + 30.999)))
    assert not holds(sc.row1_after_its_set_row(case(T0 + 37.001)))
    assert not holds(sc.row1_after_its_set_row(two_windows()))


def due(seconds):
    return f"{{'timeBase': 'TB_WORKSTATION_TIME', 'seconds': {seconds}, 'useconds': 0}}"


def test_due_seconds():
    assert sc.due_seconds(SimpleNamespace(raw=due(1234))) == 1234
    assert sc.due_seconds(SimpleNamespace(raw="none")) is None


def test_elapsed_helpers():
    sample = {"value": due(1000000 - 30 + 1050), "sec": 1050, "usec": 0}  # elapsed 30 s at FSW 1050
    assert sc.elapsed_evidence(simple(evidence={"next_due": [sample]})) == (30, 1050.0)
    assert sc.elapsed_evidence(simple()) is None
    assert holds(sc.elapsed_since_ref_start(simple(evidence={"next_due": [sample], "ref_start": [{"start": 1019.0}]})))
    assert not holds(sc.elapsed_since_ref_start(simple(evidence={"next_due": [sample], "ref_start": [{"start": 1010.0}]})))
    assert not holds(sc.elapsed_since_ref_start(simple(evidence={"next_due": [sample]})))

    def with_set(fsw):
        return simple(lambda rec: rec.ev("timeBaseSetter.TimeBaseSet", fsw, timeBase="TB_SC_TIME"),
                      {"next_due": [sample]})
    assert holds(sc.first_set_recorded("TB_SC_TIME")(with_set(1020.0)))
    assert not holds(sc.first_set_recorded("TB_PROC_TIME")(with_set(1020.0)))
    assert holds(sc.elapsed_since_first_set("TB_SC_TIME")(with_set(1021.0)))
    assert not holds(sc.elapsed_since_first_set("TB_SC_TIME")(with_set(1040.0)))
    assert not holds(sc.elapsed_since_first_set("TB_PROC_TIME")(with_set(1021.0)))


def test_not_restarted():
    def case(boot):
        return simple(lambda rec: rec.ev("prmDb.PrmFileLoadComplete", boot), {"ref_start": [{"start": 990.0}]})
    assert holds(sc.not_restarted(case(991.0)))
    assert not holds(sc.not_restarted(case(1005.0)))
    assert not holds(sc.not_restarted(simple()))


def test_due_samples():
    def case(seconds, times=(2, 4, 6, 8)):
        def build(rec):
            row_set(rec, 0, 5000, T0 + 1)
            for t, s in zip(times, seconds):
                rec.ch("seqSched.NextTimeDue", T0 + t, due(s))
        return simple(build)
    assert holds(sc.enough_due_samples(case([5000] * 4)))
    assert not holds(sc.enough_due_samples(case([5000] * 2)))
    assert not holds(sc.enough_due_samples(case([5000] * 3, times=(2, 3, 4))))
    assert holds(sc.due_steady(case([5000, 5001, 5000, 5000])))
    assert not holds(sc.due_steady(case([5000, 5001, 5002, 5003])))
    assert not holds(sc.due_steady(simple()))


def test_due_steady_across_report():
    def case(after):
        def build2(rec):
            rec.ev("timeBaseSetter.TimeBaseSet", T0 + 50)
            rec.ch("seqSched.NextTimeDue", T0 + 55, due(after))
        return two_windows(lambda rec: rec.ch("seqSched.NextTimeDue", T0 + 5, due(7000)), build2)
    assert holds(sc.due_steady_across_report(case(7001)))
    assert not holds(sc.due_steady_across_report(case(7010)))
    assert not holds(sc.due_steady_across_report(two_windows()))


def test_run_after_w1_sample():
    def case(run):
        rec = Rec()
        s = rec.mark(100, T0)
        rec.ev("seqSched.SeqRun", run, row=0)
        e = rec.mark(101, T0 + 60)
        rec.evidence["next_due"] = [{"value": due(2000), "sec": 1005, "usec": 0}]
        w = rec.win(s, e)
        return rec.ctx(w, W1=w, W3=w)
    assert holds(sc.run_after_w1_sample(10, 20)(case(1020.0)))
    assert not holds(sc.run_after_w1_sample(10, 20)(case(1030.0)))
    assert not holds(sc.run_after_w1_sample(10, 20)(case(1010.0)))
    assert holds(sc.run_after_w1_sample(10, 20)(case(1015.0)))  # at the lower limit
    assert holds(sc.run_after_w1_sample(10, 20)(case(1025.0)))  # at the upper limit
    assert not holds(sc.run_after_w1_sample(10, 20)(case(1014.999)))
    assert not holds(sc.run_after_w1_sample(10, 20)(case(1025.001)))
    no_sample = case(1020.0)
    no_sample.evidence = lambda action: []
    assert not holds(sc.run_after_w1_sample(10, 20)(no_sample))  # no NextTimeDue sample read in W1
    w = simple()
    w.evidence = lambda action: [{"value": due(2000), "sec": 1005, "usec": 0}]
    w.windows = {"W1": w.window, "W3": w.window}
    assert not holds(sc.run_after_w1_sample(10, 20)(w))  # no run of row 0 in W3


def test_last_before_marker():
    def case(value):
        def build(rec):
            rec.ch("seqSched.RowsWaiting", T0 + 5, value)
            rec.mark(150, T0 + 10)
            rec.ch("seqSched.RowsWaiting", T0 + 15, 9)
        return simple(build)
    assert holds(sc.last_before_marker("seqSched.RowsWaiting", 150, 1)(case(1)))
    assert not holds(sc.last_before_marker("seqSched.RowsWaiting", 150, 1)(case(0)))
    assert not holds(sc.last_before_marker("seqSched.RowsWaiting", 151, 1)(case(1)))


def test_active_row_is():
    def case(reported=T0 + 30, name="short.bin", sets=((1, T0 + 30),), report_row=1):
        def build(rec):
            for row, seconds in sets:
                rec.cmd("seqSched.SET_ROW", 1, [row, 0, int(seconds), 0])
            rec.ev("seqSched.ActiveRow", T0 + 12, row=report_row, fileName=name,
                   timeDue=f"{{'timeBase': 'TB_WORKSTATION_TIME', 'timeContext': 0, 'seconds': {int(reported)}, 'useconds': 0}}")
        return simple(build)
    judge = sc.active_row_is("seqSched", 1, "short.bin")
    assert holds(judge(case()))
    assert not holds(judge(case(reported=T0 + 31)))  # another time
    assert not holds(judge(case(name="long.bin")))  # another file
    assert not holds(judge(case(report_row=0)))  # no report of row 1
    assert not holds(judge(case(sets=((0, T0 + 30),))))  # no SET_ROW of row 1
    assert holds(judge(case(sets=((1, T0 + 5), (0, T0 + 9), (1, T0 + 30)))))  # the last SET_ROW of the row counts
    assert not holds(judge(case(sets=((1, T0 + 30), (1, T0 + 5)))))
    assert not holds(sc.active_row_is("seqSchedB", 1, "short.bin")(case()))  # the other instance


def test_run_gap():
    def case(second):
        def build(rec):
            rec.ev("seqSched.SeqRun", T0 + 10, row=0)
            rec.ev("seqSched.SeqRun", second, row=1)
        return simple(build)
    assert holds(sc.run_gap(0, 1, 5)(case(T0 + 16)))
    assert not holds(sc.run_gap(0, 1, 5)(case(T0 + 12)))
    assert not holds(sc.run_gap(0, 2, 5)(case(T0 + 16)))


def test_sample_while_running():
    def case(waiting, running, at=T0 + 15, apart=0.0):
        def build(rec):
            rec.ev("seqSched.SeqRun", T0 + 10, row=2)
            rec.ch("seqSched.RowsWaiting", at, waiting)
            rec.ch("seqSched.RunningRow", at + apart, running)
            rec.ev("seqSched.SeqCompleted", T0 + 20, row=2)
        return simple(build)
    assert holds(sc.sample_while_running(2, 1)(case(1, 2)))
    assert not holds(sc.sample_while_running(2, 1)(case(0, 2)))
    assert not holds(sc.sample_while_running(2, 1)(case(1, 3)))
    assert not holds(sc.sample_while_running(2, 1)(case(1, 2, at=T0 + 25)))
    # The two channels of one tick are microseconds apart; those of two ticks are not paired
    assert holds(sc.sample_while_running(2, 1)(case(1, 2, apart=0.000004)))
    assert not holds(sc.sample_while_running(2, 1)(case(1, 2, apart=1.0)))
    assert holds(sc.sample_while_running(2, 1)(case(1, 2, apart=0.499)))  # just inside the pairing limit
    assert not holds(sc.sample_while_running(2, 1)(case(1, 2, apart=0.5)))  # at it: not paired
    assert not holds(sc.sample_while_running(2, 1)(case(1, 2, at=T0 + 10)))  # at the run: not inside the span
    assert not holds(sc.sample_while_running(2, 1)(case(1, 2, at=T0 + 20)))  # at the completion
    assert not holds(sc.sample_while_running(2, 1)(simple()))  # no run, no completion
    only_run = simple(lambda rec: (rec.ev("seqSched.SeqRun", T0 + 10, row=2), rec.ch("seqSched.RowsWaiting", T0 + 15, 1),
                                   rec.ch("seqSched.RunningRow", T0 + 15, 2)))
    assert not holds(sc.sample_while_running(2, 1)(only_run))  # no completion in the window


def test_samples_b():
    def case(n):
        return simple(lambda rec: [rec.ch(sc.SAMPLE_B, T0 + 1 + i, due(1)) for i in range(n)])
    assert holds(sc.samples_b(case(2)))
    assert not holds(sc.samples_b(case(1)))
    # judged live in the first window, a later window is not recorded yet: only the helper's own window counts
    rec = Rec()
    start = rec.mark(100, T0)
    for i in range(2):
        rec.ch(sc.SAMPLE_B, T0 + 1 + i, due(1))
    w = rec.win(start, rec.mark(101, T0 + 40))
    later = rec.win(rec.mark(102, T0 + 41), rec.mark(103, T0 + 42))
    assert holds(sc.samples_b(rec.ctx(w, W=w, C=later)))
    assert not holds(sc.samples_b(rec.ctx(later, W=w, C=later)))


def test_first_set_before_test():
    def case(fsw):
        rec = Rec()
        if fsw < T0:
            rec.ev("timeBaseSetter.TimeBaseSet", fsw, timeBase="TB_SC_TIME")
        start = rec.mark(100, T0)
        if fsw >= T0:
            rec.ev("timeBaseSetter.TimeBaseSet", fsw, timeBase="TB_SC_TIME")
        return rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))
    assert holds(sc.first_set_before_test("TB_SC_TIME")(case(T0 - 5)))
    assert not holds(sc.first_set_before_test("TB_SC_TIME")(case(T0 + 5)))
    assert not holds(sc.first_set_before_test("TB_PROC_TIME")(case(T0 - 5)))


def test_up_at_marker():
    assert holds(sc.up_at_marker(30)(simple(evidence={"ref_start": [{"start": T0 - 31}]})))
    assert not holds(sc.up_at_marker(30)(simple(evidence={"ref_start": [{"start": T0 - 29}]})))
    assert not holds(sc.up_at_marker(30)(simple()))


def test_run_after_restart():
    def case(run, starts):
        return simple(lambda rec: rec.ev("seqSched.SeqRun", run, row=0), {"ref_start": starts})
    two = [{"start": 900.0}, {"start": 1010.0}]
    assert holds(sc.run_after_restart(0, 8, 12)(case(1020.0, two)))
    assert not holds(sc.run_after_restart(0, 8, 12)(case(1030.0, two)))
    assert not holds(sc.run_after_restart(0, 8, 12)(case(1020.0, two[:1])))
    # a start recorded by a later window is not the restart of this check: the second row stays the one judged
    assert holds(sc.run_after_restart(0, 8, 12)(case(1020.0, two + [{"start": 1500.0}])))


def test_loads_around_run():
    def case(second_load):
        def build(rec):
            rec.ev("seqSched.TableLoaded", T0 + 2)
            rec.ev("seqSched.SeqRun", T0 + 10, row=0)
            rec.ev("seqSched.TableLoaded", second_load)
            rec.ev("schedSeq.CS_SequenceComplete", T0 + 30, fileName="long.bin")
        return simple(build)
    assert holds(sc.loads_around_run(case(T0 + 20)))
    assert not holds(sc.loads_around_run(case(T0 + 5)))  # loaded before the run
    assert not holds(sc.loads_around_run(case(T0 + 35)))  # loaded after the sequence ended
    assert not holds(sc.loads_around_run(case(T0 + 10)))  # at the run: not after it
    assert not holds(sc.loads_around_run(case(T0 + 30)))  # at the end of the sequence: not before it
    assert holds(sc.loads_around_run(case(T0 + 10.001)))
    assert holds(sc.loads_around_run(case(T0 + 29.999)))

    def missing(what):
        def build(rec):
            rec.ev("seqSched.TableLoaded", T0 + 2)
            if what != "run":
                rec.ev("seqSched.SeqRun", T0 + 10, row=0)
            if what != "load":
                rec.ev("seqSched.TableLoaded", T0 + 20)
            if what != "complete":
                rec.ev("schedSeq.CS_SequenceComplete", T0 + 30, fileName="long.bin")
        return simple(build)
    for what in ("run", "load", "complete"):
        assert not holds(sc.loads_around_run(missing(what))), what


def test_file_unchanged():
    def case(last):
        return simple(evidence={"read_file": [{"sha256": "a" * 64, "size": 10}, {"sha256": last * 64, "size": 10}]})
    assert holds(sc.file_unchanged("read_file")(case("a")))
    assert not holds(sc.file_unchanged("read_file")(case("b")))
    assert not holds(sc.file_unchanged("read_file")(simple(evidence={"read_file": [{"sha256": "a" * 64, "size": 10}]})))
    three = [{"sha256": "a" * 64, "size": 10}] * 3  # a third reading: the check no longer knows its pair
    assert not holds(sc.file_unchanged("read_file")(simple(evidence={"read_file": three})))


def test_differing():
    assert sc.differing(sc.t_base(), sc.t_base(), sc.FIELDS) == []
    assert sc.differing(sc.t_fields(), sc.t_base(), sc.FIELDS) != []
    assert sc.differing(sc.t_base()[:15], sc.t_base(), sc.FIELDS) != []
    assert sc.differing(None, sc.t_base(), sc.FIELDS) != []
    got = sc.t_base()
    got[3]["enabled"] = True
    assert sc.differing(got, sc.t_base(), sc.FIELDS) == ["row 3 enabled: True != False"]
    assert sc.differing(got, sc.t_base(), sc.FIELDS, skip=((3, "enabled"),)) == []


def test_product_is():
    def case(rows, error=""):
        return simple(evidence={"read_dp": [{"rows": rows, "file": "a.fdp", "files": 1, "error": error}]})
    states = ["PENDING"] * 16
    good = [dict(r, state="PENDING") for r in sc.t_base()]
    assert holds(sc.product_is(sc.t_base(), states)(case(good)))
    assert not holds(sc.product_is(sc.t_fields(), states)(case(good)))
    assert not holds(sc.product_is(sc.t_base(), ["EXECUTED"] + states[1:])(case(good)))
    assert not holds(sc.product_is(sc.t_base())(case(None, "no file")))
    assert not holds(sc.product_is(sc.t_base())(simple()))
    later = {"rows": good, "file": "b.fdp", "files": 2, "error": ""}  # a second product read later in the test
    twice = simple(evidence={"read_dp": [{"rows": good, "file": "a.fdp", "files": 1, "error": ""}, later]})
    assert not holds(sc.product_is(sc.t_base(), states)(twice))


def test_new_product():
    def case(before, after):
        return simple(evidence={"count_dp": [{"files": before}], "read_dp": [{"files": after}]})
    assert holds(sc.new_product(case(1, 2)))
    assert not holds(sc.new_product(case(2, 2)))
    assert not holds(sc.new_product(simple()))
    assert not holds(sc.new_product(simple(evidence={"count_dp": [{"files": 1}], "read_dp": [{"files": 2}, {"files": 3}]})))
    assert not holds(sc.new_product(simple(evidence={"count_dp": [{"files": 1}, {"files": 1}], "read_dp": [{"files": 2}]})))


def test_table_is_final():
    def case(change=None, valid=True):
        rows = sc.t_final()
        rows[6]["seconds"] = rows[8]["seconds"] = 900
        if change:
            change(rows)
        return simple(evidence={"read_table": [{"valid": valid, "rows": rows, "error": "crc"}]})
    assert holds(sc.table_is_final(case()))
    assert not holds(sc.table_is_final(case(lambda rows: rows[5].update(enabled=False))))
    assert not holds(sc.table_is_final(case(lambda rows: rows[0].update(state="PENDING"))))
    assert not holds(sc.table_is_final(case(lambda rows: rows[6].update(seconds=sc.YEAR_2100))))  # not a past time
    assert not holds(sc.table_is_final(case(valid=False)))
    assert not holds(sc.table_is_final(simple()))
    good = case().evidence("read_table")
    assert not holds(sc.table_is_final(simple(evidence={"read_table": good + good})))  # a second reading of the table


def dispatched(rec, opcode, fsw):
    rec.ev("cmdDisp.OpCodeDispatched", fsw, f"Opcode 0x{opcode:x} dispatched to port 0")


def test_commands_dispatched():
    def case(set_row=1, no_op=0):
        rec = Rec()
        dispatched(rec, 0x1000001, T0 - 0.1)  # the opening marker's own dispatch precedes its event
        start = rec.mark(100, T0)
        rec.cmd("seqSched.SET_ROW", 0x10000a04)
        for i in range(set_row):
            dispatched(rec, 0x10000a04, T0 + 2 + i)
        for i in range(no_op):
            dispatched(rec, NO_OP_OPCODE, T0 + 10 + i)
        dispatched(rec, 0x1000001, T0 + 59.9)
        return rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))
    assert holds(HELPERS["commands_dispatched"](case()))
    assert holds(HELPERS["commands_dispatched"](case(no_op=2)))  # a sequence's own CMD_NO_OP commands
    assert not holds(HELPERS["commands_dispatched"](case(set_row=0)))  # a command sent was not dispatched
    assert not holds(HELPERS["commands_dispatched"](case(set_row=2)))  # a command dispatched twice


def test_dropped_steady():
    def case(inside, name="events.EventsDropped", before=0):
        rec = Rec()
        if before is not None:
            rec.ch(name, T0 - 50, before)
        start = rec.mark(100, T0)
        if inside is not None:
            rec.ch(name, T0 + 5, inside)
        return rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))
    assert holds(HELPERS["events_dropped_steady"](case(0)))
    assert holds(HELPERS["events_dropped_steady"](case(None)))  # sent on change: no sample means no change
    assert not holds(HELPERS["events_dropped_steady"](case(1)))
    assert not holds(HELPERS["events_dropped_steady"](case(1, before=None)))  # first sample of the start is not zero
    assert holds(HELPERS["com_dropped_steady"](case(0, "comQueue.comIngressDropped")))
    assert not holds(HELPERS["com_dropped_steady"](case(3, "comQueue.comManagedDropped")))

    def choice(inside, other=None, earlier_start=None, same_start=7, late=None):
        """The sample before the window is the last of the same channel and the same start, before the marker."""
        rec = Rec()
        rec.ch("comQueue.comIngressDropped", T0 - 50, 3)
        rec.ch("comQueue.comIngressDropped", T0 - 40, same_start)
        if earlier_start is not None:  # a start before this one, filed after this start's samples
            rec.ch("comQueue.comIngressDropped", T0 - 10, earlier_start, epoch=-1)
        if other is not None:
            rec.ch("comQueue.comManagedDropped", T0 - 30, other)  # the other channel, later than the sample wanted
        start = rec.mark(100, T0)
        rec.ch("comQueue.comIngressDropped", T0 + 5, inside)
        end = rec.mark(101, T0 + 60)
        if late is not None:
            rec.ch("comQueue.comIngressDropped", T0 + 70, late)  # after the window: not the sample before it
        return rec.ctx(rec.win(start, end))
    steady = HELPERS["com_dropped_steady"]
    assert holds(steady(choice(7)))
    assert not holds(steady(choice(3)))  # the last sample before the window, not an earlier one
    assert holds(steady(choice(7, other=9)))  # another channel's sample is not this channel's
    assert not holds(steady(choice(9, other=9)))
    assert holds(steady(choice(7, earlier_start=9)))  # a sample of an earlier start is not of this start
    assert not holds(steady(choice(9, earlier_start=9)))
    assert holds(steady(choice(7, late=9)))  # a sample after the closing marker is not before the window
    assert not holds(steady(choice(9, late=9)))


def test_every_predicate_has_a_known_answer_test():
    """A new helper of seqsched_common.py without a test here fails this test."""
    here = Path(__file__).read_text(encoding="utf-8")
    not_predicates = {"reqs", "time_value", "fsw_now", "this_start", "standard_requires", "run_time", "bracket",
                      "override_span", "records_of", "elapsed_of", "due_samples_after_row_set", "first_set_of_start",
                      "row_event_time", "abs_time", "span_events", "_raw_args", "manifest_in_hand", "bracket_both", "t_base", "t_fields", "t_final"}
    names = [n for n, f in vars(sc).items() if callable(f) and getattr(f, "__module__", "") == sc.__name__]
    missing = [n for n in names if n not in not_predicates and f"sc.{n}(" not in here]
    assert not missing, missing


# One case per condition whose and/or flip survived the reviewer's mutation run (harness review R10-1, R10-2)

def test_sequencers_idle_counts_this_start_up_to_the_opening_marker_only():
    def case(fsw, epoch):
        rec = Rec()
        start = rec.mark(100, T0)
        rec.ev("schedSeq.CS_SequenceLoaded", fsw, epoch=epoch)
        return rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))
    assert holds(sc.sequencers_idle(case(T0 + 5, 0)))  # loaded inside the window: after the opening marker
    assert holds(sc.sequencers_idle(case(T0 - 20, 1)))  # loaded by another start of the image
    assert not holds(sc.sequencers_idle(case(T0 - 20, 0)))


def test_plan_params_ignores_other_commands_and_sets_inside_the_window():
    rec = Rec()
    rec.cmd("seqSched.SET_ENABLED", 3, raw="['5']")  # not a plan parameter
    start = rec.mark(100, T0)
    rec.cmd("cmdDisp.CMD_NO_OP_STRING", 1, [100])
    rec.cmd("seqSched.LATE_TOLERANCE_SEC_PRM_SET", 5, raw="['5']")  # set by the window itself
    ctx = rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))
    ctx.window.cmds = rec.commands[2:]
    assert holds(sc.plan_params(ctx))


def test_run_not_before_row_time_takes_the_set_row_of_its_own_row():
    def case(other_first):
        def build(rec):
            if other_first:
                rec.cmd("seqSched.SET_ROW", 7, [1, 0, 1030, 0])  # another row, a later time
                rec.cmd("seqSched.SET_ENABLED", 3, [0, 0, 1030, 0])  # another command with the row as first argument
            rec.cmd("seqSched.SET_ROW", 7, [0, 0, 1010, 0])
            row_set(rec, 0, 1010, T0 + 1)
            rec.ev("seqSched.SeqRun", 1010.2, row=0)
        return simple(build)
    assert holds(sc.run_not_before_row_time(0)(case(False)))
    assert holds(sc.run_not_before_row_time(0)(case(True)))


def test_override_span_needs_a_set_and_a_clear():
    assert sc.override_span(simple(lambda rec: rec.ev("timeBaseSetter.TimeBaseSet", T0 + 10, base=SC_TIME))) is None
    assert sc.override_span(simple(lambda rec: rec.ev("timeBaseSetter.TimeBaseCleared", T0 + 30))) is None
    assert sc.override_span(span()) == (T0 + 10, T0 + 30)


def test_row1_after_its_set_row_needs_the_run_and_the_command():
    def sets(rec):
        rec.cmd("seqSched.SET_ROW", 0xa04)
        rec.cmd("seqSched.SET_ROW", 0xa04)
        rec.ev("cmdDisp.OpCodeDispatched", T0 + 2, "Opcode 0xa04 dispatched to port 0")
        rec.ev("cmdDisp.OpCodeDispatched", T0 + 4, "Opcode 0xa04 dispatched to port 0")
    assert not holds(sc.row1_after_its_set_row(two_windows(sets, None)))  # no run of row 1
    assert not holds(sc.row1_after_its_set_row(  # the run, but no second SET_ROW in W1
        two_windows(None, lambda rec: rec.ev("seqSched.SeqRun", T0 + 34, row=1))))


def test_elapsed_of():
    def bench(rows):
        return SimpleNamespace(action=lambda name: rows)
    assert sc.elapsed_of(bench([])) == 0
    assert sc.elapsed_of(bench([None])) == 0
    assert sc.elapsed_of(bench([{"value": "{'seconds': '1000900'}", "sec": 1000}])) == 1000000 - (1000900 - 1000)


def test_dropped_steady_array_of_zeros_and_a_sample_at_the_opening_marker():
    def case(inside, before=None, at=T0 + 5):
        rec = Rec()
        if before is not None:
            rec.ch("comQueue.comIngressDropped", T0 - 50, before)
        start = rec.mark(100, T0)
        rec.ch("comQueue.comIngressDropped", at, inside)
        return rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))
    steady = HELPERS["com_dropped_steady"]
    assert holds(steady(case("[0, 0]")))  # the first sample of the start: zero in every element
    assert not holds(steady(case("[0, 10]")))
    # a sample at the time of the opening marker is of the window, not the sample before it
    assert not holds(steady(case("[0, 1]", before="[0, 0]", at=T0)))


def test_row_time_is_the_reading_plus_the_offset_in_workstation_time():
    assert json.loads(sc.row_time(5000, 18)) == {"timeBase": "TB_WORKSTATION_TIME", "timeContext": 0,
                                                 "seconds": 5018, "useconds": 0}
    # rows of one group keep their spacing: nothing is read between them
    assert json.loads(sc.row_time(5000, 26))["seconds"] - json.loads(sc.row_time(5000, 23))["seconds"] == 3


def test_rows_set_ahead_judges_each_absolute_workstation_set_row_by_its_dispatch_time():
    ws = "TB_WORKSTATION_TIME"
    opcode = {"seqSched": 0x10005004, "seqSchedB": 0x10006004}

    def case(*rows, raw=None):
        """rows: (instance, FSW time of the dispatch or None for no dispatch event, time base, seconds, mode)"""
        rec = Rec()
        start = rec.mark(100, T0)
        for inst, fsw, base, seconds, mode in rows:
            time = {"timeBase": base, "timeContext": 0, "seconds": seconds, "useconds": 0}
            rec.cmd(inst + ".SET_ROW", opcode[inst], [0, 0, seconds, 0],
                    raw if raw is not None else str([0, time, mode, "short.bin", "WAIT_FOR_RUNNING"]))
            if fsw is not None:
                rec.ev("cmdDisp.OpCodeDispatched", fsw, f"Opcode 0x{opcode[inst]:x} dispatched to port 3")
        return rec.ctx(rec.win(start, rec.mark(101, T0 + 60)))

    def ahead(h):
        return sc.rows_set_ahead(h)
    due = int(T0) + 6
    assert holds(ahead(case()))  # no row command: nothing was set late
    assert holds(ahead(case(("seqSched", T0 + 5.9, ws, due, "ABSOLUTE"))))
    assert not holds(ahead(case(("seqSched", T0 + 6.0, ws, due, "ABSOLUTE"))))  # dispatched at its time
    assert not holds(ahead(case(("seqSched", T0 + 8.4, ws, due, "ABSOLUTE"))))  # the L6-2 case
    assert not holds(ahead(case(("seqSched", None, ws, due, "ABSOLUTE"))))  # no dispatch event: not shown
    assert not holds(ahead(case(("seqSched", T0 + 1, ws, due, "ABSOLUTE"), raw="not a list")))
    # each command is judged by its own dispatch, per instance
    second_late = case(("seqSched", T0 + 2, ws, due, "ABSOLUTE"), ("seqSched", T0 + 9, ws, due, "ABSOLUTE"))
    assert not holds(ahead(second_late)) and "1 of 2" not in ahead(second_late)[1]
    late_b = case(("seqSched", T0 + 2, ws, due, "ABSOLUTE"), ("seqSchedB", T0 + 9, ws, due, "ABSOLUTE"))
    assert not holds(ahead(late_b)) and "seqSchedB.SET_ROW row 0" in ahead(late_b)[1]
    assert holds(ahead(case(("seqSched", T0 + 2, ws, due, "ABSOLUTE"), ("seqSchedB", T0 + 3, ws, due, "ABSOLUTE"))))
    # not judged: another time base, a relative row, a year-2100 row; they still take their place among the dispatches
    assert holds(ahead(case(("seqSched", T0 + 9, "TB_SC_TIME", due, "ABSOLUTE"))))
    assert holds(ahead(case(("seqSched", T0 + 9, ws, 5, "START_RELATIVE"))))
    assert holds(ahead(case(("seqSched", T0 + 9, ws, sc.YEAR_2100, "ABSOLUTE"))))
    mixed = case(("seqSched", T0 + 9, ws, sc.YEAR_2100, "ABSOLUTE"), ("seqSched", T0 + 10, ws, int(T0) + 20, "ABSOLUTE"))
    assert holds(ahead(mixed)) and "1 of 1" in ahead(mixed)[1]
    assert not holds(ahead(case(("seqSched", T0 + 30, ws, sc.YEAR_2100, "ABSOLUTE"),
                                ("seqSched", T0 + 31, ws, int(T0) + 20, "ABSOLUTE"))))


def test_events_received_in_time():
    def case(lags, marker_lag=0.5):
        rec = Rec()
        start = rec.mark(100, T0)
        start.recv = None if marker_lag is None else T0 + marker_lag
        for i, lag in enumerate(lags):
            rec.ev("seqSched.RowSet", T0 + 5 + i).recv = None if lag is None else T0 + 5 + i + lag
        end = rec.mark(101, T0 + 60)
        end.recv = T0 + 60.5
        rec.ev("seqSched.RowSet", T0 + 70).recv = T0 + 99.0  # after the window: not judged
        return rec.ctx(rec.win(start, end))
    assert holds(HELPERS["events_received_in_time"](case([0.2, 10.0])))  # the bound itself holds
    assert holds(HELPERS["events_received_in_time"](case([])))
    assert holds(HELPERS["events_received_in_time"](case([-0.3])))  # received "before" its tag: clock jitter
    assert not holds(HELPERS["events_received_in_time"](case([0.2, 10.01])))
    assert not holds(HELPERS["events_received_in_time"](case([0.2, None])))  # an event without a receive time
    assert not holds(HELPERS["events_received_in_time"](case([0.2], marker_lag=11.0)))  # the markers are judged
    assert not holds(HELPERS["events_received_in_time"](case([0.2], marker_lag=None)))
