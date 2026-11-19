"""SeqScheduler system test L0-2 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


def test_L0_2(window, send, bench, check, require):
    with window(110, 111):
        require("STD")
        require("no image start-up event in the window", no_start_up)
        standard_requires(require)
        bracket(bench)
        send("Ref.timeBaseSetter.SET_TIME_BASE", ["TB_SC_TIME"])
        try:
            bench.wait(2)
            # short.bin ends within the tick that starts it: count from before the command that runs it
            done = bench.count_events("schedSeq.CS_SequenceComplete")
            send("Ref.schedSeq.CS_RUN", ["short.bin", "NO_BLOCK"])
            send("CdhCore.cmdDisp.CMD_NO_OP")
            send("CdhCore.health.HLTH_ENABLE", ["ENABLED"])
            send("Ref.seqSched.LOAD_TABLE")
            bench.wait_event("schedSeq.CS_SequenceComplete", 5, base=done)
            send("Ref.timeBaseSetter.SET_TIME_BASE", ["TB_SC_TIME"])
            bench.wait(3)
        finally:
            send("Ref.timeBaseSetter.CLEAR_TIME_BASE")  # also after a failed procedure: the override must not reach the next test
        bracket(bench)
        check("event timeBaseSetter.TimeBaseSet(timeBase=TB_SC_TIME) count == 2")
        check("event timeBaseSetter.TimeBaseCleared count == 1")
        require(SPAN_RECORDS, span_has_records)
        check(SPAN_BASE, span_base)
        check(SPAN_SECONDS, span_seconds)
        check(SPAN_RECEIVE, span_receive)
        check("the time tag of the second timeBaseSetter.TimeBaseSet event carries TB_SC_TIME", second_set_base)
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
