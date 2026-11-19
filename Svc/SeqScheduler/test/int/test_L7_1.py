"""SeqScheduler system test L7-1 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


@pytest.mark.req(*reqs("032 035 019 027 025 056 023 031 028 029 005 002"))
def test_L7_1(window, send, bench, check, require):
    with window(700, 701):
        require("STD")
        standard_requires(require, fresh=True)
        bench.action("write_T_BASE")
        send("Ref.seqSched.LOAD_TABLE")
        bench.action("count_dp")
        send("Ref.seqSched.SET_ROW_ENABLED", [5, True])
        send("Ref.seqSched.SET_ROW", [5, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "short2.bin",
                              "SKIP_WHEN_RUNNING"])
        send("Ref.seqSched.SET_ROW_ENABLED", [6, True])
        send("Ref.seqSched.SET_ROW_ENABLED", [7, True])
        send("Ref.seqSched.SET_ROW", [8, abs_time(bench, 6), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [6, abs_time(bench, 6), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SET_ROW", [7, abs_time(bench, 9), "ABSOLUTE", "short.bin", "WAIT_FOR_RUNNING"])
        bench.wait(14)
        send("Ref.seqSched.SET_ROW_ENABLED", [6, False])
        send("Ref.seqSched.SET_ROW_ENABLED", [6, True])
        send("Ref.seqSched.SET_ROW", [7, time_value("TB_WORKSTATION_TIME", YEAR_2100), "ABSOLUTE", "short.bin",
                              "WAIT_FOR_RUNNING"])
        send("Ref.seqSched.SAVE_TABLE")
        send("Ref.seqSched.SET_ROW_ENABLED", [4, True])
        send("Ref.seqSched.LOAD_TABLE")
        bench.wait(3)
        send("Ref.seqSched.DUMP_TABLE")
        bench.wait_event("dpWriter.FileWritten")
        bracket(bench)
        bench.action("read_table")
        bench.action("read_dp")
        check("ABS")
        check("event seqSched.SeqRun(row=6, fileName=short.bin) count == 1")
        check("event seqSched.SeqRun(row=7, fileName=short.bin) count == 1")
        check("event seqSched.TableSaved count == 1")
        check("event seqSched.TableDumped count == 1")
        check("event seqSched.TableLoaded count == 2")
        check("event seqSched.RowLate count == 0")
        check("event seqSched.RowSkippedDisabled(row=8) count == 1")
        check("event seqSched.RowSkippedDisabled count == 1")
        check("last(seqSched.RowsExecuted) == 0")
        check("last(seqSched.RowsSkipped) == 2")
        check("last(seqSched.RowsPending) == 14")
        check("table file on the bench file system == T-FINAL: valid checksum; row 5 short2.bin SKIP_WHEN_RUNNING "
              "enabled, rows 6 and 7 enabled, row 8 disabled, every other row and field as T-BASE, no row state",
              table_is_final)
        require("the data product file decoded was written in this test", new_product)
        states = ["PENDING"] * 16
        states[6] = states[8] = "SKIPPED"
        check("data product file decodes to T-FINAL with row states: rows 6 and 8 SKIPPED, every other row PENDING",
              product_is(t_final(), states, PAST_TIMES))
