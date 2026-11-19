"""SeqScheduler system test L2-8 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("016 013 012"))
def test_L2_8(window, send, bench, check, require):
    with window(262, 263):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [1, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [2, True])
        send("Ref.seqSched.SET_ROW", [2, abs_time(bench, 34), "ABSOLUTE", "short2.bin", "SKIP_WHEN_RUNNING"])
        send("Ref.seqSched.SET_ROW", [1, abs_time(bench, 12), "ABSOLUTE", "short.bin", "SKIP_WHEN_RUNNING"])
        send("Ref.seqSched.SET_ROW", [0, abs_time(bench, 6), "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
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
