"""Known-answer tests of the helper predicates of the SeqScheduler system tests.

Each helper of ../int/seqsched_common.py and ../int/planted_faults.py is given a small hand-made
record on which it must hold and one on which it must not. Run: `pytest Svc/SeqScheduler/test/kat`.
"""
import sys
from pathlib import Path
from types import SimpleNamespace

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "int"))

import seqsched_common as sc  # noqa: E402
from fth.registry import load_helpers_module  # noqa: E402
from fth.replay import ChannelRow, CommandRow, EventRow  # noqa: E402

HELPERS = load_helpers_module(Path(__file__).resolve().parent.parent / "int" / "planted_faults.py").helpers
NO_OP_OPCODE = 0x1000000  # CdhCore.cmdDisp.CMD_NO_OP

T0 = 1000.0  # FSW time of the opening marker
WS, SC_TIME = 2, 3


class Win:
    """The part of fth.replay.Window the helpers use."""

    def __init__(self, ev, ch, cmds):
        self.ev, self.ch, self.cmds = ev, ch, cmds
        self.start_marker, self.end_marker = ev[0], ev[-1]

    def events(self, short=None, **args):
        return [e for e in self.ev if (short is None or e.short == short)
                and all(e.args.get(k) == v for k, v in args.items())]

    def channel(self, short):
        return [c for c in self.ch if c.short == short]


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
        return self.ev("cmdDisp.NoOpStringReceived", fsw, id=marker)

    def win(self, start, end):
        """Window between two marker events: events and channels by time, commands given by the caller."""
        a, b = self.events.index(start), self.events.index(end)
        return Win(self.events[a:b + 1], [c for c in self.channels if start.fsw <= c.fsw <= end.fsw], list(self.commands))

    def ctx(self, window, **windows):
        record = SimpleNamespace(events=self.events, channels=self.channels, commands=self.commands)
        return SimpleNamespace(record=record, window=window, windows=dict(windows) or {"W": window},
                               evidence=lambda action: self.evidence.get(action, []))


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
    def reading(age, at):
        return {"age_s": age, "sec": int(at), "usec": int(round((at - int(at)) * 1e6))}

    inside, inside2 = reading(0.4, T0 + 5), reading(1.0, T0 + 20.5)
    assert holds(sc.fresh_times()(simple(evidence={"fsw_now": [inside, inside2]})))
    assert not holds(sc.fresh_times()(simple(evidence={"fsw_now": [inside, reading(1.5, T0 + 20)]})))
    # A stale reading of another window of the test is not this window's: before START, at or after END
    assert holds(sc.fresh_times()(simple(evidence={"fsw_now": [reading(1.5, T0 - 1), inside, reading(1.5, T0 + 60)]})))
    assert holds(sc.fresh_times()(simple(evidence={"fsw_now": [reading(0.2, T0)]})))  # at START: this window's
    assert not holds(sc.fresh_times()(simple(evidence={"fsw_now": [reading(1.5, T0)]})))
    # No reading of its own: a failed reading, readings of other windows only, none at all
    assert not holds(sc.fresh_times()(simple(evidence={"fsw_now": [None]})))
    assert not holds(sc.fresh_times()(simple(evidence={"fsw_now": [reading(0.1, T0 + 60)]})))
    assert not holds(sc.fresh_times()(simple()))
    # A failed reading has no time and no window; fsw_now stops the test there
    assert holds(sc.fresh_times()(simple(evidence={"fsw_now": [inside, None]})))
    # The 4 s form of the windows opened by the END marker of the window before
    assert holds(sc.fresh_times(4.0)(simple(evidence={"fsw_now": [reading(1.6, T0 + 1), reading(4.0, T0 + 2)]})))
    assert not holds(sc.fresh_times(4.0)(simple(evidence={"fsw_now": [reading(1.6, T0 + 1), reading(4.1, T0 + 2)]})))


def test_fsw_now_raises_on_a_failed_reading():
    class Bench:
        def __init__(self, rows, dry_run=False):
            self.rows, self.dry_run = rows, dry_run

        def action(self, name):
            return self.rows

    assert sc.fsw_now(Bench([{"sec": 7, "usec": 0, "age_s": 0.1}])) == 7
    assert sc.fsw_now(Bench([], dry_run=True)) == 0
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
    assert not holds(sc.plan_params(with_set("['5']")))


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
    assert holds(sc.run_after(0, "seqSched.SET_ROW", 8, 12)(case(T0 + 12)))
    assert not holds(sc.run_after(0, "seqSched.SET_ROW", 8, 12)(case(T0 + 9)))
    assert not holds(sc.run_after(0, "seqSched.SET_ROW", 8, 12)(case(T0 + 15)))
    assert not holds(sc.run_after(0, "seqSched.LOAD_TABLE", 8, 12)(case(T0 + 12)))


def test_version_strings_match():
    def case(text, wanted=({"describe": ["608087211"]},)):
        return simple(lambda rec: rec.ev("version.FrameworkVersion", T0 + 1, text),
                      {"manifest_versions": list(wanted)})
    assert holds(sc.version_strings_match(case("Framework version: v4.0.0-12-g608087211")))
    assert not holds(sc.version_strings_match(case("Framework version: bogus")))
    assert not holds(sc.version_strings_match(case("Framework version: 608087211", wanted=[{"describe": []}])))
    assert not holds(sc.version_strings_match(case("Framework version: 608087211", wanted=[])))
    assert not holds(sc.version_strings_match(simple(evidence={"manifest_versions": [{"describe": ["608087211"]}]})))


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
    def case(waiting, running, at=T0 + 15):
        def build(rec):
            rec.ev("seqSched.SeqRun", T0 + 10, row=2)
            rec.ch("seqSched.RowsWaiting", at, waiting)
            rec.ch("seqSched.RunningRow", at, running)
            rec.ev("seqSched.SeqCompleted", T0 + 20, row=2)
        return simple(build)
    assert holds(sc.sample_while_running(2, 1)(case(1, 2)))
    assert not holds(sc.sample_while_running(2, 1)(case(0, 2)))
    assert not holds(sc.sample_while_running(2, 1)(case(1, 3)))
    assert not holds(sc.sample_while_running(2, 1)(case(1, 2, at=T0 + 25)))


def test_samples_b():
    def case(n):
        return simple(lambda rec: [rec.ch(sc.SAMPLE_B, T0 + 1 + i, due(1)) for i in range(n)])
    assert holds(sc.samples_b(case(2)))
    assert not holds(sc.samples_b(case(1)))


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


def test_file_unchanged():
    def case(last):
        return simple(evidence={"read_file": [{"sha256": "a" * 64, "size": 10}, {"sha256": last * 64, "size": 10}]})
    assert holds(sc.file_unchanged("read_file")(case("a")))
    assert not holds(sc.file_unchanged("read_file")(case("b")))
    assert not holds(sc.file_unchanged("read_file")(simple(evidence={"read_file": [{"sha256": "a" * 64, "size": 10}]})))


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


def test_new_product():
    def case(before, after):
        return simple(evidence={"count_dp": [{"files": before}], "read_dp": [{"files": after}]})
    assert holds(sc.new_product(case(1, 2)))
    assert not holds(sc.new_product(case(2, 2)))
    assert not holds(sc.new_product(simple()))


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


def dispatched(rec, opcode, fsw):
    rec.ev("cmdDisp.OpCodeDispatched", fsw, f"Opcode 0x{opcode:x} dispatched to port 0")


def test_commands_dispatched():
    def case(set_row=1, no_op=0):
        rec = Rec()
        dispatched(rec, 0x1000001, T0 - 0.1)  # the opening marker's own dispatch precedes its event
        start = rec.mark(100, T0)
        rec.cmd("cmdDisp.CMD_NO_OP_STRING", 0x1000001, [100])
        rec.cmd("seqSched.SET_ROW", 0x10000a04)
        for i in range(set_row):
            dispatched(rec, 0x10000a04, T0 + 2 + i)
        for i in range(no_op):
            dispatched(rec, NO_OP_OPCODE, T0 + 10 + i)
        rec.cmd("cmdDisp.CMD_NO_OP_STRING", 0x1000001, [101])
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


def test_every_predicate_has_a_known_answer_test():
    """A new helper of seqsched_common.py without a test here fails this test."""
    here = Path(__file__).read_text(encoding="utf-8")
    not_predicates = {"reqs", "time_value", "fsw_now", "this_start", "standard_requires", "run_time", "bracket",
                      "override_span", "records_of", "elapsed_of", "due_samples_after_row_set", "first_set_of_start",
                      "row_event_time", "abs_row", "settle", "bracket_both", "t_base", "t_fields", "t_final"}
    names = [n for n, f in vars(sc).items() if callable(f) and getattr(f, "__module__", "") == sc.__name__]
    missing = [n for n in names if n not in not_predicates and f"sc.{n}(" not in here]
    assert not missing, missing
