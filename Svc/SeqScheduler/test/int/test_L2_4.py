"""SeqScheduler system test L2-4 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("008 041 047"))
def test_L2_4(window, send, bench, check, require):
    with window(230, 231):
        require("STD")
        standard_requires(require)
        bench.action("ref_start")
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW", [0, time_value("TB_NONE", 1000000), "START_RELATIVE", "short.bin", "WAIT_FOR_RUNNING"])
        bench.wait(4)
        bracket(bench)
        elapsed = elapsed_of(bench)
        check("last(seqSched.NextRow) == 0")
        check("event seqSched.SeqRun count == 0")
        require("at least 3 NextTimeDue samples time-tagged after RowSet(row=0), spanning >= 4 s", enough_due_samples)
        check("seconds of those NextTimeDue samples are equal within 1 s", due_steady)
        check("elapsed is within 3 s of FSW time at the sample less the bench-recorded start of the Ref process",
              elapsed_since_ref_start)
        require("Ref process not restarted since the bench recorded its start", not_restarted)
    with window(231, 232):
        require("STD")
        # read again: the markers between the windows take longer than the 3 s the pass rule allows for
        elapsed = elapsed_of(bench)
        send("Ref.seqSched.SET_ROW", [0, time_value("TB_NONE", elapsed + 12), "START_RELATIVE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(18)
        bracket(bench)
        check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("row 0 run no earlier than its delay after the start of the Ref process", run_not_before_delay(0))
        check("row 0 run between 10 and 15 s after the W2 NextTimeDue sample", run_after_sample(10, 15, reading=1))
