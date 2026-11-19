"""SeqScheduler system test L5-1 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("003 005"))
def test_L5_1(window, send, bench, check, require):
    try:
        with window(500, 501):
            require("STD-ERR")
            standard_requires(require, fresh=True)
            bench.action("write_T_BASE")
            bench.action("write_T_TRUNC")
            send(S + ".LOAD_TABLE")
            send(S + ".SET_ROW_ENABLED", [0, True])
            send(S + ".SET_ROW", [0] + abs_row(bench, 15, "short.bin", "WAIT_FOR_RUNNING"))
            send(S + ".TABLE_FILE_PRM_SET", ["missing.tbl"])
            send(S + ".LOAD_TABLE")
            send(S + ".TABLE_FILE_PRM_SET", [T_TRUNC_FILE])
            send(S + ".LOAD_TABLE")
            bench.wait(20)
            bracket(bench)
            check("event cmdDisp.OpCodeError count == 2")
            check("event seqSched.TableLoadFailed(reason=OPEN) count == 1")
            check("event seqSched.TableFileMissing count == 0")
            check("event seqSched.TableLoadFailed(reason=SIZE) count == 1")
            check("event seqSched.TableLoaded count == 1")
            check("event seqSched.CmdRejected count == 0")
            check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
            check("event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1")
            check("last(seqSched.TableLoaded) == True")
            check("last(seqSched.RowsPending) == 15")
            check("last(seqSched.RowsExecuted) == 1")
    finally:
        # clean-up window: TABLE_FILE back to the plan value and T-BASE loaded
        with window(501, 502):
            require("both markers present")
            send(S + ".TABLE_FILE_PRM_SET", [T_BASE_FILE])
            send(S + ".LOAD_TABLE")
            bracket(bench)
            check("event seqSched.TableLoaded count == 1")
