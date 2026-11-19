"""SeqScheduler system test L2-12 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("009 011 047 061"))
def test_L2_12(window, send, bench, check, require):
    with window(293, 294):
        require("STD")
        standard_requires(require)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW", [0, time_value("TB_SC_TIME", 1000000), "TIME_BASE_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bracket(bench)
        elapsed = elapsed_of(bench)
        send("Ref.seqSched.SET_ROW", [0, time_value("TB_SC_TIME", elapsed + 20), "TIME_BASE_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        send("Ref.timeBaseSetter.SET_TIME_BASE", ["TB_SC_TIME"])
        bench.wait(3)
        send("Ref.timeBaseSetter.CLEAR_TIME_BASE")
        bench.wait(20)
        bracket(bench)
        check("ABS")
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSched.RowLate count == 0")
        check("last(seqSched.RowsExecuted) == 1")
        check("row 0 run between 14 and 22 s after the second SET_ROW", run_after(0, "seqSched.SET_ROW", 14, 22, 1))
        check("elapsed is within 3 s of FSW time at the sample less the time of this start's first "
              "timeBaseSetter.TimeBaseSet(TB_SC_TIME) event", elapsed_since_first_set("TB_SC_TIME"))
        require("the first TimeBaseSet(TB_SC_TIME) of this start is in the record, in an earlier test",
                first_set_before_test("TB_SC_TIME"))
