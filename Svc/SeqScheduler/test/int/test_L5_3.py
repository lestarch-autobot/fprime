"""SeqScheduler system test L5-3 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("003 005"))
def test_L5_3(window, send, bench, check, require):
    try:
        with window(520, 521):
            require("STD-ERR")
            standard_requires(require, fresh=True)
            bench.action("write_T_BASE")
            bench.action("write_T_BADCRC")
            bench.action("hash_T_BADCRC")
            send("Ref.seqSched.LOAD_TABLE")
            send("Ref.seqSched.SET_ROW_ENABLED", [0, True])
            send("Ref.seqSched.SET_ROW", [0, abs_time(bench, 15), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
            send("Ref.seqSched.TABLE_FILE_PRM_SET", ["t_badcrc.tbl"])
            send("Ref.seqSched.LOAD_TABLE")
            bench.wait(20)
            bracket(bench)
            bench.action("hash_T_BADCRC")
            require(SET_AHEAD, rows_set_ahead)
            check("event cmdDisp.OpCodeError count == 1")
            check("event seqSched.TableLoadFailed(reason=CHECKSUM) count == 1")
            check("event seqSched.TableLoadFailed count == 1")
            check("event seqSched.TableLoaded count == 1")
            check("event seqSched.SeqRun(row=0, fileName=short.bin) count == 1")
            check("event seqSched.SeqCompleted(row=0, response=OK) count == 1")
            check("event seqSched.TableSaved count == 0")
            check("last(seqSched.TableLoaded) == True")
            check("last(seqSched.RowsExecuted) == 1")
            check("file T-BADCRC unchanged on the bench file system", file_unchanged("hash_T_BADCRC"))
    finally:
        # clean-up window: TABLE_FILE back to the plan value and T-BASE loaded
        with window(521, 522):
            require("both markers present")
            send("Ref.seqSched.TABLE_FILE_PRM_SET", ["seq_sched.tbl"])
            send("Ref.seqSched.LOAD_TABLE")
            bracket(bench)
            check("event seqSched.TableLoaded count == 1")
