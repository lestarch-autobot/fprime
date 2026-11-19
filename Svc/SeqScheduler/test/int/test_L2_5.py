"""SeqScheduler system test L2-5 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("009 011 061"))
def test_L2_5(window, send, bench, check, require):
    with window(240, 241):
        require("STD")
        standard_requires(require)
        bench.action("write_T_BASE")
        send(S + ".LOAD_TABLE")
        send(T + ".SET_TIME_BASE", ["TB_PROC_TIME"])
        bench.wait(3)
        send(T + ".CLEAR_TIME_BASE")
        send(S + ".SET_ROW_ENABLED", [0, True])
        send(S + ".SET_ROW", [0, time_value("TB_PROC_TIME", 1000000), "TIME_BASE_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bracket(bench)
        elapsed = elapsed_of(bench)
        check("last(seqSched.NextRow) == 0")
        check("event timeBaseSetter.TimeBaseSet(timeBase=TB_PROC_TIME) count == 1")
        check("elapsed is within 3 s of FSW time at the sample less the time of this start's first "
              "timeBaseSetter.TimeBaseSet(TB_PROC_TIME) event", elapsed_since_first_set("TB_PROC_TIME"))
        require("the first TimeBaseSet(TB_PROC_TIME) of this start is in the record, in this test or an earlier one",
                first_set_recorded("TB_PROC_TIME"))
    with window(241, 242):
        require("STD")
        send(T + ".SET_TIME_BASE", ["TB_PROC_TIME"])
        bench.wait(3)
        send(T + ".CLEAR_TIME_BASE")
        bracket(bench)
        check("seconds of NextTimeDue are equal within 1 s across the second TB_PROC_TIME report",
              due_steady_across_report)
    with window(242, 243):
        require("STD")
        send(S + ".SET_ROW", [0, time_value("TB_PROC_TIME", elapsed + 20), "TIME_BASE_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(20)
        bracket(bench)
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("row 0 run between 11 and 21 s after the W1 NextTimeDue sample", run_after_w1_sample(11, 21))
