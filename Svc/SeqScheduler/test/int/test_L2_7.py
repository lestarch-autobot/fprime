"""SeqScheduler system test L2-7 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("015 013 060"))
def test_L2_7(window, send, bench, check, require):
    with window(260, 261):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [1, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [2, True])
        send("Ref.seqSched.SET_ROW", [1, abs_time(bench, 16), "ABSOLUTE", "short.bin", "CANCEL_RUNNING"])
        send("Ref.seqSched.SET_ROW", [2, abs_time(bench, 11), "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [0, abs_time(bench, 6), "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
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
