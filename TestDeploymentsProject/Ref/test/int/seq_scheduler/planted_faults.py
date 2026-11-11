"""Planted faults for the SEQSCHED plan (Svc.SeqScheduler on Ref).

Loaded by ``fth`` through ``plan.json: helpers_module``. Each planted fault receives
``(test_id, command_name, args)`` and returns rewritten ``args`` (kind ``rewrite``) or a list of
``(command_name, args)`` to send instead (kind ``insert``). ``fails`` names, per test and verbatim,
the checks the stimulus test declares that the fault must break. ``faults-menu.json`` is generated
FROM these decorators (``fth planted-faults-menu plan.json``) and is never hand-written.
"""
from fth.registry import helper, planted_fault


@helper("commands_sent_minus_seq")
def commands_sent_minus_seq(ctx):
    """STD completeness on Ref: cmdDisp also dispatches the NO-OPs issued by the stimulus sequences
    (SEQ_A / SEQ_B / SEQ_L_*), which are not in commands.jsonl; subtract them (messages starting with SEQ_;
    the numeric window markers are not counted)."""
    w = ctx.window
    dispatched = len(w.events("CdhCore.cmdDisp.OpCodeDispatched"))
    seq_noops = [e for e in w.events("CdhCore.cmdDisp.NoOpStringReceived") if str(e.args.get("message", "")).startswith("SEQ_")]
    sent = w.commands_sent
    return sent == dispatched - len(seq_noops), f"sent={sent} dispatched={dispatched} seq_noops={len(seq_noops)}"


def _is(command_name, short):
    return command_name == short or command_name.endswith("." + short)


@planted_fault("wrong_sequence_name", applies_to=["L2-1"], cls="stimulus",
      mechanism="SET_ROW_SEQUENCE names seqs/missing.bin: the row dispatches on time, CmdSequencer answers CS_FileNotFound + seqDone(EXECUTION_ERROR) with no start, so the unconfirmed row FAILS at once (RowFailed); SEQ_A never runs",
      fails={"L2-1": ["event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_A) count == 1", "event cmdSeq.CS_SequenceComplete count == 1", "event seqScheduler.RowCompleted(index=0) count == 1", "event seqScheduler.RowDispatched(index=0) precedes event seqScheduler.RowCompleted(index=0)", "last(seqScheduler.RowsComplete) == 1", "event cmdSeq.CS_FileNotFound count == 0"]})
def wrong_sequence_name(test_id, command_name, args):
    if test_id == "L2-1" and _is(command_name, "seqScheduler.SET_ROW_SEQUENCE"):
        return [args[0], "seqs/missing.bin"]
    return args


@planted_fault("late_absolute", applies_to=["L2-2"], cls="stimulus",
      mechanism="SET_ROW_TIME absolute seconds rewritten from N+4 to N-62: when TB_SC_TIME appears the row is 72 s late and is skipped instead of dispatched",
      fails={"L2-2": ["W2: event seqScheduler.RowDispatched(index=1) count == 1", "W2: event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_B) count == 1", "W2: event seqScheduler.RowCompleted(index=1) count == 1", "W2: event seqScheduler.RowSkippedLate count == 0", "t(event seqScheduler.RowDispatched(index=1)) - t(timeBaseOverride.SET_TIME_BASE sent) <= 3 s"]})
def late_absolute(test_id, command_name, args):
    if test_id == "L2-2" and _is(command_name, "seqScheduler.SET_ROW_TIME"):
        return args[:3] + [args[3] - 66, args[4]]
    return args


@planted_fault("disturb_disable", applies_to=["L2-3"], cls="uut-state", kind="insert",
      mechanism="a DISABLE_ROW(2) is inserted right after ENABLE_ROW(2): the row is disabled in the table when the base appears, so it never dispatches",
      fails={"L2-3": ["W1: last(seqScheduler.NextRow) == 2", "W2: event seqScheduler.RowDispatched(index=2) count == 1", "W2: event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_A) count == 1", "W2: dispatch not early (check function: t(RowDispatched) - t(SET_TIME_BASE sent) >= 4 s)", "t(event seqScheduler.RowDispatched(index=2)) - t(timeBaseOverride.SET_TIME_BASE sent) <= 8 s"]})
def disturb_disable(test_id, command_name, args):
    if test_id == "L2-3" and _is(command_name, "seqScheduler.ENABLE_ROW") and args == [2]:
        return [(command_name, args), ("seqScheduler.DISABLE_ROW", [2])]
    return None


@planted_fault("drop_wait_flag", applies_to=["L2-4"], cls="stimulus",
      mechanism="SET_ROW_FLAGS(4, true, false) sent as (4, false, false): row 4 is due while row 3 is DISPATCHED and, lacking waitPrevious, FAILS without dispatch (RowFailed(4, BUSY), REQ-041) — never dispatched, never completed",
      fails={"L2-4": ["event seqScheduler.RowDispatched(index=4) count == 1", "event seqScheduler.RowDispatched(index=3) precedes event seqScheduler.RowDispatched(index=4)", "event seqScheduler.RowCompleted(index=3) precedes event seqScheduler.RowDispatched(index=4)", "event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_L_END) precedes event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_A)", "event seqScheduler.RowCompleted(index=4) count == 1", "t(event seqScheduler.RowDispatched(index=4)) - t(seqScheduler.ENABLE_ROW sent) <= 18 s"]})
def drop_wait_flag(test_id, command_name, args):
    if test_id == "L2-4" and _is(command_name, "seqScheduler.SET_ROW_FLAGS") and args[0] == 4:
        return [4, False, False]
    return args


@planted_fault("no_cancel_flag", applies_to=["L2-5"], cls="stimulus",
      mechanism="SET_ROW_FLAGS(6, false, true) sent as (6, false, false): row 6 is due while row 5 is DISPATCHED and FAILS with BUSY instead of cancelling; the long sequence runs to SEQ_L_END",
      fails={"L2-5": ["event seqScheduler.RowCancelled(index=6, running=5) count == 1", "event cmdSeq.CS_SequenceCanceled count == 1", "event seqScheduler.RowFailed(index=5) count == 1", "event seqScheduler.RowFailed(index=5) precedes event seqScheduler.RowDispatched(index=6)", "event seqScheduler.RowDispatched(index=6) count == 1", "event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_B) count == 1", "event seqScheduler.RowCompleted(index=6) count == 1", "event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_L_END) count == 0", "t(event seqScheduler.RowDispatched(index=6)) - t(event seqScheduler.RowCancelled(index=6, running=5)) <= 1 s"]})
def no_cancel_flag(test_id, command_name, args):
    if test_id == "L2-5" and _is(command_name, "seqScheduler.SET_ROW_FLAGS") and args[0] == 6:
        return [6, False, False]
    return args


@planted_fault("persist_late_row", applies_to=["L3-1"], cls="uut-state", kind="insert",
      mechanism="before SAVE_TABLE the harness inserts SET_ROW_TIME(0, START_RELATIVE, TB_NONE, 500, 0): the saved table carries a 500 s offset, so nothing dispatches in the 12 s after restart",
      fails={"L3-1": ["W2: event seqScheduler.RowDispatched(index=0) count == 1", "W2: event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_A) count == 1", "W2: event seqScheduler.RowCompleted(index=0) count == 1", "W2: last(seqScheduler.RowsPending) == 7", "W2: last(seqScheduler.RowsComplete) == 1"]})
def persist_late_row(test_id, command_name, args):
    if test_id == "L3-1" and _is(command_name, "seqScheduler.SAVE_TABLE"):
        return [("seqScheduler.SET_ROW_TIME", [0, "START_RELATIVE", "TB_NONE", 500, 0]), (command_name, args)]
    return None


@planted_fault("fresh_absolute", applies_to=["L5-1"], cls="stimulus",
      mechanism="SET_ROW_TIME seconds rewritten from N-60 to N+3: the row is on time and dispatches instead of being skipped late",
      fails={"L5-1": ["event seqScheduler.RowSkippedLate(index=7) count == 1", "event seqScheduler.RowDispatched count == 0", "event CdhCore.cmdDisp.NoOpStringReceived(message=SEQ_A) count == 0", "last(seqScheduler.RowsSkipped) == 1", "t(event seqScheduler.RowSkippedLate(index=7)) - t(seqScheduler.ENABLE_ROW sent) <= 3 s"]})
def fresh_absolute(test_id, command_name, args):
    if test_id == "L5-1" and _is(command_name, "seqScheduler.SET_ROW_TIME"):
        return args[:3] + [args[3] + 63, args[4]]
    return args
