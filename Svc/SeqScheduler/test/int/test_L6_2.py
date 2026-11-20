"""SeqScheduler system test L6-2 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("018 040"))
def test_L6_2(window, send, bench, check, require):
    with window(610, 611):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        bracket(bench)
        send("Ref.schedSeq.CS_RUN", ["short.bin", "NO_BLOCK"])
        bench.wait(4)
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW", [0, abs_time(bench, 12), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        bench.wait(18)
        bracket(bench)
        # not ABS: the ground's own CS_RUN makes one SeqDoneUnexpected, judged below
        require(SET_AHEAD, rows_set_ahead)
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
