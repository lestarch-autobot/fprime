"""SeqScheduler system test L2-2 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("006 012 020 040"))
def test_L2_2(window, send, bench, check, require):
    with window(210, 211):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW", [0, time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 10), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(16)
        bracket(bench)
        require(SET_AHEAD, rows_set_ahead)
        check("ABS")
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceLoaded(fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSched.SeqRun(row=0) precedes event seqSched.SeqCompleted(row=0)")
        check("last(seqSched.RowsExecuted) == 1")
        check("row 0 run no earlier than the time set in the row", run_not_before_row_time(0))
        check("row 0 run no later than 3 s after the time set in the row", run_within_row_time(0, 3))
