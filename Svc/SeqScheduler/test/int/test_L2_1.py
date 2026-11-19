"""SeqScheduler system test L2-1 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("019 035 041 002 004"))
def test_L2_1(window, send, bench, check, require):
    try:
        with window(200, 201):
            require("STD")
            standard_requires(require)
            bracket(bench)
            bench.action("count_dp")
            bench.action("write_T_FIELDS")
            send("Ref.seqSched.LOAD_TABLE")
            send("Ref.seqSched.DUMP_TABLE")
            bench.wait_event("dpWriter.FileWritten")
            bracket(bench)
            bench.action("read_dp")
            check("ABS")
            check("event seqSched.TableLoaded count == 1")
            check("event seqSched.TableDumped count == 1")
            check("last(seqSched.TableLoaded) == True")
            check("last(seqSched.RowsPending) == 16")
            check("last(seqSched.RowsExecuted) == 0")
            check("last(seqSched.RowsSkipped) == 0")
            check("last(seqSched.RowsWaiting) == 0")
            check("last(seqSched.NextRow) == 16")
            check("last(seqSched.RunningRow) == 16")
            require("the data product file decoded was written in this test", new_product)
            check("data product file decodes to table T-FIELDS, every field of every row as authored",
                  product_is(t_fields()))
    finally:
        with window(201, 202):
            require("both markers present")
            bench.action("write_T_BASE")
            send("Ref.seqSched.LOAD_TABLE")
            bracket(bench)
            check("event seqSched.TableLoaded count == 1")
