"""SeqScheduler system test L2-17 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("058"))
def test_L2_17(window, send, bench, check, require):
    try:
        with window(330, 331):
            require("STD")
            standard_requires(require, fresh=True)
            require("first(seqSched.Enabled) == True")
            bench.action("write_T_BASE")
            send("Ref.seqSched.LOAD_TABLE")
            send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
            send("Ref.seqSched.SET_ROW_ENABLED", [1, True])
            due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 12)
            send("Ref.seqSched.SET_ROW", [1, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
            send("Ref.seqSched.SET_ROW", [0, due, "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
            bench.wait(17)
            send("Ref.seqSched.SET_ENABLED", [False])
            send("Ref.seqSched.SET_ENABLED", [True])
            bench.wait(28)
            bracket(bench)
            require(SET_AHEAD, rows_set_ahead)
            check("event seqSched.SeqRun(row=0, fileName=long.bin) count == 1")
            check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
            check("event seqSched.EnabledSet(enabled=False) count == 1")
            check("event seqSched.EnabledSet(enabled=True) count == 1")
            require(RUNNING_AT_ENABLES, enables_while_running(0))
            check("event seqSched.RowSkippedDisabled(row=1) count == 1")
            check("event seqSched.RowSkippedDisabled(row=1, cause=WAITING_AT_DISABLE) count == 1")
            check("event seqSched.RowSkippedDisabled(row=1) precedes event seqSched.EnabledSet(enabled=False)")
            check("event seqSched.RowSkippedDisabled count == 1")
            check("event seqSched.SeqRun(row=1) count == 0")
            check("event seqSched.SeqRun count == 1")
            check("event seqSched.RowLate count == 0")
            check("event seqSched.SeqCancelled count == 0")
            check("event seqSched.RowSkipsNotReported count == 0")
            check("last(seqSched.Enabled) == True")
            check("last(seqSched.RowsSkipped) == 1")
            check("last(seqSched.RowsExecuted) == 1")
            check("last(seqSched.RowsWaiting) == 0")
    finally:
        # clean-up window: a failed procedure must not leave the scheduler disabled for the next test
        with window(331, 332):
            require("both markers present")
            send("Ref.seqSched.SET_ENABLED", [True])
            bracket(bench)
            check("last(seqSched.Enabled) == True")
