"""SeqScheduler system test L2-10 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("042 012 040 007 022 026 034"))
def test_L2_10(window, send, bench, check, require):
    try:
        with window(280, 281):
            require("STD-ERR")
            standard_requires(require, fresh=True)
            bench.action("write_T_BASE")
            send("Ref.seqSched.LOAD_TABLE")
            bench.action("write_T_BASE_B")
            send("Ref.seqSchedB.LOAD_TABLE")
            send("Ref.seqSched.SET_SEQ_ENABLED", ["short.bin", True])
            send("Ref.seqSchedB.SET_ROW_ENABLED", [0, True])
            send("Ref.seqSchedB.SET_ROW_ENABLED", [1, True])
            send("Ref.seqSchedB.SET_ROW_ENABLED", [16, True])
            send("Ref.seqSchedB.SET_ROW", [1, time_value("TB_SC_TIME", fsw_now(bench) + 6), "ABSOLUTE", "short.bin",
                                  "WAIT_FOR_RUNNING"])
            due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 12)
            send("Ref.seqSchedB.SET_ROW", [2, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
            send("Ref.seqSchedB.SET_ROW", [0, due, "ABSOLUTE", "short2.bin", "WAIT_FOR_RUNNING"])
            send("Ref.seqSched.SET_ROW", [0, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
            bench.wait(22)
            bracket(bench)
            bench.wait_samples(SAMPLE_B, 2)
            check("ABS")
            check("event cmdDisp.OpCodeError count == 1")
            check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
            check("event seqSchedB.SeqRun(row=0, fileName=short2.bin) count == 1")
            check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
            check("event schedSeqB.CS_SequenceComplete(fileName=short2.bin) count == 1")
            check("event schedSeq.CS_SequenceLoaded(fileName=short2.bin) count == 0")
            check("event schedSeqB.CS_SequenceLoaded(fileName=short.bin) count == 0")
            check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
            check("event seqSchedB.SeqCompleted(row=0, response=OK) count == 1")
            check("event seqSched.SeqEnabledSet(fileName=short.bin, enabled=True, rows=16) count == 1")
            check("event seqSched.SeqRun count == 1")
            check("event seqSchedB.SeqRun count == 1")
            check("event seqSchedB.CmdRejected(reason=ROW_INDEX) count == 1")
            # "event seqSched.CmdRejected count == 0" of the plan is one of the ABS checks above
            check("event seqSchedB.RowLate count == 0")
            check("event seqSchedB.RowSkippedDisabled(row=2) count == 1")
            check("event seqSchedB.RowSkippedDisabled count == 1")
            check("last(seqSched.RowsExecuted) == 1")
            check("last(seqSched.RowsPending) == 15")
            check("last(seqSchedB.RowsExecuted) == 1")
            check("last(seqSchedB.RowsSkipped) == 1")
            check("last(seqSchedB.RowsPending) == 14")
            require("SeqSchedB samples >= 2", samples_b)
    finally:
        # clean-up window: a sequence left running by a failed procedure ends before the next test
        with window(281, 282):
            require("both markers present")
            bench.wait(25)
            bracket(bench)
            check("SeqSched samples >= 2")
