"""SeqScheduler system test L2-11 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("006 012 020 040 027 028 029"))
def test_L2_11(window, send, bench, check, require):
    try:
        with window(290, 291):
            require("STD")
            standard_requires(require, fresh=True)
            bench.action("write_T_BASE")
            send("Ref.seqSched.LOAD_TABLE")
            send("Ref.seqSched.SET_ROW_ENABLED", [3, True])
            send("Ref.seqSched.SET_ROW", [3, abs_time(bench, 12), "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
            bench.wait(18)
            send("Ref.seqSched.SET_ROW_ENABLED", [3, False])
            send("Ref.seqSched.SET_ROW_ENABLED", [3, True])
            bench.wait(4)
            bracket(bench)
            require(SET_AHEAD, rows_set_ahead)
            check("ABS")
            check("event seqSched.RowSet(row=3, mode=ABSOLUTE, fileName=short2.bin, busyPolicy=WAIT_FOR_RUNNING) "
                  "count == 1")
            check("event seqSched.SeqRun(row=3, fileName=short2.bin) count == 1")
            check("event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1")
            check("event seqSched.SeqCompleted(row=3, response=OK) count == 1")
            check("event seqSched.SeqRun count == 1")
            check("event seqSched.RowEnabledSet(row=3) count == 3")
            check("last(seqSched.RowsExecuted) == 1")
            check("last(seqSched.RowsPending) == 15")
            check("row 3 run no earlier than the time set in the row", run_not_before_row_time(3))
            check("row 3 run no later than 3 s after the time set in the row", run_within_row_time(3, 3))
        with window(291, 292):
            require("STD")
            standard_requires(require, fresh=4)
            send("Ref.seqSched.SET_ROW", [3, abs_time(bench, 12), "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
            bench.wait(16)
            bracket(bench)
            require(SET_AHEAD, rows_set_ahead)
            check("ABS")
            check("event seqSched.SeqRun(row=3, fileName=short2.bin) count == 1")
            check("event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1")
            check("last(seqSched.RowsExecuted) == 1")
    finally:
        # clean-up window: a sequence left running by a failed procedure ends before the next test
        with window(292, 299):
            require("both markers present")
            require(RECEIVED, received_in_time)
            bench.wait(25)
            bracket(bench)
            check("SeqSched samples >= 2")
