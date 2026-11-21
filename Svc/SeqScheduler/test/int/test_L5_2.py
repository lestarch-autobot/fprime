"""SeqScheduler system test L5-2 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("030 040"))
def test_L5_2(window, send, bench, check, require):
    with window(510, 511):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW", [0, abs_time(bench, 12), "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
        bench.wait(16)
        send("Ref.seqSched.SET_ROW", [0, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW_ENABLED", [0, False])
        send("Ref.seqSched.SET_SEQ_ENABLED", ["short.bin", False])
        send("Ref.seqSched.LOAD_TABLE")
        bench.wait(24)
        bracket(bench)
        require(SET_AHEAD, rows_set_ahead)
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
