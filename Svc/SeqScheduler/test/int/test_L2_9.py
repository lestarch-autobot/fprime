"""SeqScheduler system test L2-9 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("025 026 027 022 029 012"))
def test_L2_9(window, send, bench, check, require):
    with window(270, 271):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [1, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [2, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [3, True])
        send("Ref.seqSched.SET_ROW", [2, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "long.bin",
                              "WAIT_FOR_RUNNING"])
        due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 12)
        send("Ref.seqSched.SET_ROW", [3, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [1, due, "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [0, due, "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_SEQ_ENABLED", ["long.bin", False])
        send("Ref.seqSched.SET_ROW_ENABLED", [3, False])
        bench.wait(18)
        bracket(bench)
        require(SET_AHEAD, rows_set_ahead)
        check("ABS")
        check("event seqSched.RowSet(row=0, fileName=short2.bin) count == 1")
        check("event seqSched.RowSet count == 4")
        check("event seqSched.SeqEnabledSet(fileName=long.bin, enabled=False, rows=2) count == 1")
        check("event seqSched.RowEnabledSet(row=3, enabled=False) count == 1")
        check("event seqSched.SeqRun(row=0, fileName=short2.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1")
        check("event seqSched.SeqRun count == 1")
        check("event seqSched.RowLate count == 0")
        check("event seqSched.RowSkippedDisabled(row=1) count == 1")
        check("event seqSched.RowSkippedDisabled(row=3) count == 1")
        check("event seqSched.RowSkippedDisabled count == 2")
        check("event schedSeq.CS_SequenceLoaded(fileName=long.bin) count == 0")
        check("last(seqSched.RowsExecuted) == 1")
        check("last(seqSched.RowsSkipped) == 2")
        check("last(seqSched.RowsPending) == 13")
