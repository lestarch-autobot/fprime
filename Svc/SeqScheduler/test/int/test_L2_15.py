"""SeqScheduler system test L2-15 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("057 058"))
def test_L2_15(window, send, bench, check, require):
    try:
        with window(310, 311):
            require("STD")
            standard_requires(require, fresh=True)
            require("first(seqSched.Enabled) == True")
            bench.action("write_T_BASE")
            send("Ref.seqSched.LOAD_TABLE")
            send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
            send("Ref.seqSched.SET_ROW_ENABLED", [1, True])
            send("Ref.seqSched.SET_ROW", [0] + abs_row(bench, 12, "short.bin", "WAIT_FOR_RUNNING"))
            send("Ref.seqSched.SET_ENABLED", [False])
            send("Ref.seqSched.REPORT_ACTIVE_ROW")
            bench.wait(18)
            send("Ref.seqSched.SET_ENABLED", [True])
            send("Ref.seqSched.SET_ROW", [1] + abs_row(bench, 10, "short.bin", "WAIT_FOR_RUNNING"))
            send("Ref.seqSched.REPORT_ACTIVE_ROW")
            bench.wait(16)
            bracket(bench)
            check("event seqSched.EnabledSet(enabled=False) count == 1")
            check("event seqSched.EnabledSet(enabled=True) count == 1")
            check("event seqSched.NoActiveRow count == 1")
            check("event seqSched.ActiveRow(row=1) count == 1")
            check("event seqSched.ActiveRow count == 1")
            check("event seqSched.RowSkippedDisabled(row=0) count == 1")
            check("event seqSched.SeqRun(row=0) count == 0")
            check("event seqSched.SeqRun(row=1) count == 1")
            check("event seqSched.SeqRun count == 1")
            check("event schedSeq.CS_SequenceComplete count == 1")
            check("event seqSched.RowLate count == 0")
            check("event seqSched.NoActiveRow precedes event seqSched.RowSkippedDisabled(row=0)")
            check("last(seqSched.Enabled) == True")
            check("last(seqSched.RowsSkipped) == 1")
            check("last(seqSched.RowsExecuted) == 1")
            check("last(seqSched.LastExecutedRow) == 1")
    except BaseException:
        # the plan's `finally`: a failed procedure must not leave the scheduler disabled for the next test
        send("Ref.seqSched.SET_ENABLED", [True])
        raise
