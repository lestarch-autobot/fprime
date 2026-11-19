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
        base = fsw_now(bench)  # one reading for the five rows: their spacing does not depend on the commands
        send("Ref.seqSchedB.SET_ROW", [1, row_time(base, 28), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSchedB.SET_ROW", [3, row_time(base, 25), "ABSOLUTE", "short.bin", "SKIP_WHEN_RUNNING"])
        due = row_time(base, 22)
        send("Ref.seqSchedB.SET_ROW", [4, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSchedB.SET_ROW", [2, due, "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSchedB.SET_ROW", [0, row_time(base, 18), "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
        bench.wait(44)
        bracket_both(bench)
        require(SET_AHEAD, rows_set_ahead)
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
