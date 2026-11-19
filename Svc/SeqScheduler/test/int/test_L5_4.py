"""SeqScheduler system test L5-4 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("030 018 042 028"))
def test_L5_4(window, send, bench, check, require):
    with window(530, 531):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE_B")
        send(B + ".LOAD_TABLE")
        send(QB + ".CS_RUN", ["short.bin", "NO_BLOCK"])
        bench.wait(4)
        send(B + ".SET_ROW_ENABLED", [0, True])
        send(B + ".SET_ROW", [0] + abs_row(bench, 6, "long.bin", "WAIT_FOR_RUNNING"))
        bench.wait(10)
        send(B + ".SET_ROW_ENABLED", [0, False])
        send(B + ".SET_ROW", [0, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "short2.bin",
                              "WAIT_FOR_RUNNING"])
        bench.wait(24)
        bracket_both(bench)
        check("event schedSeqB.CS_SequenceComplete(fileName=short.bin) count == 1")
        check("event seqSchedB.SeqDoneUnexpected count == 1")
        check("event seqSchedB.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event schedSeqB.CS_SequenceComplete(fileName=long.bin) count == 1")
        check("event schedSeqB.CS_SequenceCanceled count == 0")
        check("event seqSchedB.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSchedB.SeqCompleted count == 1")
        check("event seqSchedB.SeqDoneUnexpected precedes event seqSchedB.SeqRun(row=0)")
        check("event seqSchedB.SeqRun(row=0) precedes event seqSchedB.RowSet(row=0, fileName=short2.bin)")
        check("event seqSchedB.RowSet(row=0, fileName=short2.bin) precedes "
              "event schedSeqB.CS_SequenceComplete(fileName=long.bin)")
        check("last(seqSchedB.RunningRow) == 16")
        check("last(seqSchedB.RowsExecuted) == 0")
        check("event seqSched.SeqDoneUnexpected count == 0")
        check("event seqSched.SeqRun count == 0")
        require("SeqSchedB samples >= 2 in every window", samples_b)
