"""SeqScheduler system test L2-16 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("057 058 042"))
def test_L2_16(window, send, bench, check, require):
    try:
        with window(320, 321):
            require("STD")
            standard_requires(require, fresh=True)
            require("first(seqSchedB.Enabled) == True")
            bench.action("write_T_BASE_B")
            send("Ref.seqSchedB.LOAD_TABLE")
            send("Ref.seqSchedB.SET_ROW_ENABLED", [0, True])
            send("Ref.seqSchedB.SET_ROW_ENABLED", [1, True])
            send("Ref.seqSchedB.SET_ROW", [0, abs_time(bench, 12), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
            send("Ref.seqSchedB.SET_ENABLED", [False])
            send("Ref.seqSchedB.REPORT_ACTIVE_ROW")
            bench.wait(18)
            send("Ref.seqSchedB.SET_ENABLED", [True])
            send("Ref.seqSchedB.SET_ROW", [1, abs_time(bench, 10), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
            send("Ref.seqSchedB.REPORT_ACTIVE_ROW")
            bench.wait(16)
            bracket_both(bench)
            require(SET_AHEAD, rows_set_ahead)
            check("event seqSchedB.EnabledSet(enabled=False) count == 1")
            check("event seqSchedB.EnabledSet(enabled=True) count == 1")
            check("event seqSchedB.NoActiveRow count == 1")
            check("event seqSchedB.ActiveRow(row=1) count == 1")
            check("event seqSchedB.ActiveRow count == 1")
            check(ACTIVE_ROW.format("seqSchedB"), active_row_is("seqSchedB", 1, "short.bin"))
            check("event seqSchedB.RowSkippedDisabled(row=0) count == 1")
            check("event seqSchedB.SeqRun(row=0) count == 0")
            check("event seqSchedB.SeqRun(row=1) count == 1")
            check("event seqSchedB.SeqRun count == 1")
            check("event schedSeqB.CS_SequenceComplete count == 1")
            check("event seqSchedB.RowLate count == 0")
            check("event seqSchedB.NoActiveRow precedes event seqSchedB.RowSkippedDisabled(row=0)")
            check("last(seqSchedB.Enabled) == True")
            check("last(seqSchedB.RowsSkipped) == 1")
            check("last(seqSchedB.RowsExecuted) == 1")
            check("last(seqSchedB.LastExecutedRow) == 1")
            check("event seqSched.SeqRun count == 0")
            check("event seqSched.EnabledSet count == 0")
            check("last(seqSched.Enabled) == True")
            require("SeqSchedB samples >= 2", samples_b)
    finally:
        # clean-up window: a failed procedure must not leave the scheduler disabled for the next test
        with window(321, 322):
            require("both markers present")
            send("Ref.seqSchedB.SET_ENABLED", [True])
            bracket_both(bench)
            require("SeqSchedB samples >= 2", samples_b)
            check("last(seqSchedB.Enabled) == True")
