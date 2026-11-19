"""SeqScheduler system test L6-1 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("034 004 035"))
def test_L6_1(window, send, bench, check, require):
    with window(600, 601):
        require("STD-ERR")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        bracket(bench)
        bench.action("count_dp")
        send("Ref.seqSched.SET_ROW_ENABLED", [16, True])
        send("Ref.seqSched.SET_ROW", [16, abs_time(bench, 5), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [0, abs_time(bench, 5), "ABSOLUTE", "", "WAIT_FOR_RUNNING"])
        late = json.loads(time_value("TB_WORKSTATION_TIME", fsw_now(bench) + 5))
        late["useconds"] = 1000000
        send("Ref.seqSched.SET_ROW", [0, json.dumps(late), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_SEQ_ENABLED", ["nosuch.bin", True])
        send("Ref.seqSched.SET_SEQ_ENABLED", ["", True])
        send("Ref.seqSched.DUMP_TABLE")
        bench.wait(8)
        bracket(bench)
        bench.action("read_dp")
        check("event cmdDisp.OpCodeError count == 6")
        check("event seqSched.CmdRejected(reason=ROW_INDEX) count == 2")
        check("event seqSched.CmdRejected(reason=FILE_NAME) count == 2")
        check("event seqSched.CmdRejected(reason=TIME) count == 1")
        check("event seqSched.CmdRejected(reason=NO_MATCH) count == 1")
        check("event seqSched.RowEnabledSet count == 0")
        check("event seqSched.RowSet count == 0")
        check("event seqSched.SeqEnabledSet count == 0")
        check("event seqSched.SeqRun count == 0")
        check("last(seqSched.RowsPending) == 16")
        require("the data product file decoded was written in this test", new_product)
        check("data product file decodes to table T-BASE, all rows PENDING", product_is(t_base(), ["PENDING"] * 16))
