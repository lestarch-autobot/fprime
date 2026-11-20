"""SeqScheduler system test L2-3 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("007 021 006"))
def test_L2_3(window, send, bench, check, require):
    with window(220, 221):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [1, True])
        send("Ref.seqSched.SET_ROW", [0, time_value("TB_SC_TIME", fsw_now(bench) + 11), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [1, time_value("TB_SC_TIME", fsw_now(bench) + 30), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(12)
        bracket(bench)
        check("event seqSched.SeqRun count == 0")
        check("event seqSched.RowLate count == 0")
        check("last(seqSched.RowsPending) == 16")
        check("last(seqSched.NextRow) == 16")
    with window(221, 222):
        require("STD")
        send("Ref.timeBaseSetter.SET_TIME_BASE", ["TB_SC_TIME"])
        try:
            bench.wait(24)
            bracket(bench)
        finally:
            send("Ref.timeBaseSetter.CLEAR_TIME_BASE")  # also after a failed procedure
        check("event seqSched.RowLate(row=0) count == 1")
        check("event seqSched.SeqRun(row=0) count == 0")
        check("event seqSched.SeqRun(row=1, fileName=short.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("last(seqSched.RowsSkipped) == 1")
        check("event timeBaseSetter.TimeBaseSet(timeBase=TB_SC_TIME) count == 1")
        check("event timeBaseSetter.TimeBaseCleared count == 1")
        check("row 1 run between 27 and 33 s after its SET_ROW", row1_after_its_set_row)
