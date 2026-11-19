"""SeqScheduler system test L2-14 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("016 045 014 013 017 042"))
def test_L2_14(window, send, bench, check, require):
    with window(297, 298):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE_B")
        send("Ref.seqSchedB.LOAD_TABLE")
        send("Ref.seqSchedB.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSchedB.SET_ROW_ENABLED", [1, True])
        send("Ref.seqSchedB.SET_ROW_ENABLED", [2, True])
        send("Ref.seqSchedB.SET_ROW_ENABLED", [3, True])
        send("Ref.seqSchedB.SET_ROW_ENABLED", [4, True])
        send("Ref.seqSchedB.SET_ROW", [1, abs_time(bench, 16), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSchedB.SET_ROW", [3, abs_time(bench, 13), "ABSOLUTE", "short.bin", "SKIP_WHEN_RUNNING"])
        due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 10)
        send("Ref.seqSchedB.SET_ROW", [4, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSchedB.SET_ROW", [2, due, "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSchedB.SET_ROW", [0, abs_time(bench, 6), "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
        bench.wait(44)
        bracket_both(bench)
        check("event seqSchedB.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event seqSchedB.RowSkippedBusy(row=3, runningRow=0) count == 1")
        check("event seqSchedB.SeqRun(row=3) count == 0")
        check("event seqSchedB.SeqRun(row=2, fileName=short2.bin) count == 1")
        check("event seqSchedB.SeqRun(row=4, fileName=short.bin) count == 1")
        check("event seqSchedB.SeqRun(row=1, fileName=short.bin) count == 1")
        check("event seqSchedB.SeqCompleted(row=0) precedes event seqSchedB.SeqRun(row=2)")
        check("event seqSchedB.SeqCompleted(row=2) precedes event seqSchedB.SeqRun(row=4)")
        check("event seqSchedB.SeqCompleted(row=4) precedes event seqSchedB.SeqRun(row=1)")
        check("event schedSeqB.CS_SequenceComplete(fileName=long.bin) count == 1")
        check("event schedSeqB.CS_SequenceCanceled count == 0")
        check("event seqSchedB.RowLate count == 0")
        check("event seqSchedB.RowSkippedByCancel count == 0")
        check("last(seqSchedB.RowsExecuted) == 4")
        check("last(seqSchedB.RowsSkipped) == 1")
        check("event seqSched.SeqRun count == 0")
        check("event seqSched.RowSkippedBusy count == 0")
        require("SeqSchedB samples >= 2", samples_b)
