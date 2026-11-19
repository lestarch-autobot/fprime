"""SeqScheduler system test L2-6 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("013 014 017 045 012 040 041"))
def test_L2_6(window, send, bench, check, require):
    with window(250, 251):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [1, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [2, True])
        due = time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 12)
        send("Ref.seqSched.SET_ROW", [2, due, "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [1, due, "ABSOLUTE", "missing.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [0, due, "ABSOLUTE", "long.bin", "WAIT_FOR_RUNNING"])
        bench.wait(17)
        send("CdhCore.cmdDisp.CMD_NO_OP_STRING", [252])
        bench.wait(23)
        bracket(bench)
        require(SET_AHEAD, rows_set_ahead)
        check("event seqSched.SeqRun(row=0, fileName=long.bin) count == 1")
        check("event seqSched.SeqRun(row=1, fileName=missing.bin) count == 1")
        check("event seqSched.SeqRun(row=2, fileName=short.bin) count == 1")
        check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
        check("event seqSched.SeqCompleted(row=1, response=EXECUTION_ERROR) count == 1")
        check("event seqSched.SeqCompleted(row=2, response=OK) count == 1")
        check("last(seqSched.RowsExecuted) before marker 252 == 0", last_before_marker("seqSched.RowsExecuted", 252, 0))
        check("last(seqSched.RunningRow) before marker 252 == 0", last_before_marker("seqSched.RunningRow", 252, 0))
        check("event seqSched.SeqCompleted(row=0) precedes event seqSched.SeqRun(row=1)")
        check("event seqSched.SeqCompleted(row=1) precedes event seqSched.SeqRun(row=2)")
        check("event seqSched.RowLate count == 0")
        check("event seqSched.SeqCancelled count == 0")
        check("event seqSched.RowSkippedBusy count == 0")
        check("event schedSeq.CS_FileNotFound(fileName=missing.bin) count == 1")
        check("event schedSeq.CS_SequenceComplete(fileName=long.bin) precedes "
              "event schedSeq.CS_FileNotFound(fileName=missing.bin)")
        check("row 1 run at least 15 s after row 0", run_gap(0, 1, 15))
        check("last(seqSched.RowsExecuted) == 3")
        check("last(seqSched.RowsWaiting) == 0")
        check("a SeqSched sample time-tagged between SeqRun(row=0) and SeqCompleted(row=0) holds RowsWaiting == 2 "
              "and RunningRow == 0", sample_while_running(0, 2))
