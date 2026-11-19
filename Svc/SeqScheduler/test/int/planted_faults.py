"""Planted faults of the SeqScheduler system-test plan (QUAL-SEQSCHED).

Each fault rewrites or inserts commands of the drawn test only. `fails` lists, verbatim, the
named checks of that test the fault breaks; `fth planted-faults-menu plan.json` generates the
menu from these declarations.
"""
import json

from fth.registry import planted_fault

S = "Ref.seqSched"
B = "Ref.seqSchedB"
T = "Ref.timeBaseSetter"
# Path of table file T-BASE on the bench file system; bound to the bench configuration in the harness phase.
T_BASE_PATH = "seq_sched.tbl"
LATE_S = 30

L2_2_NOT_RUN = [
    "event seqSched.SeqRun(row=0, fileName=short.bin) count == 1",
    "event schedSeq.CS_SequenceLoaded(fileName=short.bin) count == 1",
    "event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1",
    "event seqSched.SeqCompleted(row=0, response=OK) count == 1",
    "event seqSched.SeqRun(row=0) precedes event seqSched.SeqCompleted(row=0)",
    "last(seqSched.RowsExecuted) == 1",
    "t(event seqSched.SeqRun(row=0)) - t(seqSched.SET_ROW sent) <= 12 s",
]
# SET_ROW arguments: row, time, mode, fileName, busyPolicy
ROW, TIME, MODE, FILE, POLICY = range(5)


@planted_fault(
    "row_time_late",
    applies_to=["L2-2", "L2-11"],
    cls="stimulus",
    mechanism="the first SET_ROW of the test is sent with the row time 30 s later than planned; the row is not run inside its window",
    fails={
        "L2-2": L2_2_NOT_RUN,
        "L2-11": [
            "event seqSched.SeqRun(row=3, fileName=short2.bin) count == 1",
            "event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1",
            "event seqSched.SeqCompleted(row=3, response=OK) count == 1",
            "event seqSched.SeqRun count == 1",
            "last(seqSched.RowsExecuted) == 1",
            "last(seqSched.RowsPending) == 15",
            "row 3 run no later than 10 s after SET_ROW",
        ],
    },
)
def row_time_late(test_id, command_name, args):
    if command_name == S + ".SET_ROW" and not _seen(test_id, "row_time_late"):
        time = json.loads(args[TIME])
        time["seconds"] = int(time["seconds"]) + LATE_S
        return [args[ROW], json.dumps(time)] + list(args[MODE:])
    return None


_SEEN = set()


def _seen(test_id, name):
    """True from the second call for one test and fault: the fault acts on the first matching command only."""
    key = (test_id, name)
    if key in _SEEN:
        return True
    _SEEN.add(key)
    return False


@planted_fault(
    "disable_row",
    applies_to=["L2-2", "L7-1"],
    cls="uut-state",
    kind="insert",
    mechanism="SET_ROW_ENABLED(row, False) is sent after the SET_ROW of row 0 (L2-2) or row 6 (L7-1); the row reaches its time disabled and is skipped",
    fails={
        "L2-2": L2_2_NOT_RUN,
        "L7-1": [
            "event seqSched.SeqRun(row=6, fileName=short.bin) count == 1",
            "event seqSched.RowSkippedDisabled count == 1",
        ],
    },
)
def disable_row(test_id, command_name, args):
    if command_name == S + ".SET_ROW" and int(args[ROW]) in (0, 6):
        return [(command_name, args), (S + ".SET_ROW_ENABLED", [args[ROW], False])]
    return None


@planted_fault(
    "wrong_seq",
    applies_to=["L2-11"],
    cls="stimulus",
    mechanism="every SET_ROW naming short2.bin is sent with long.bin; the row holds and runs the wrong file",
    fails={
        "L2-11": [
            "event seqSched.RowSet(row=3, mode=ABSOLUTE, fileName=short2.bin, busyPolicy=WAIT_FOR_RUNNING) count == 1",
            "event seqSched.SeqRun(row=3, fileName=short2.bin) count == 1",
            "event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1",
            "event seqSched.SeqCompleted(row=3, response=OK) count == 1",
            "event seqSched.SeqRun(row=3, fileName=short2.bin) count == 1",
            "event schedSeq.CS_SequenceComplete(fileName=short2.bin) count == 1",
        ],
    },
)
def wrong_seq(test_id, command_name, args):
    if command_name == S + ".SET_ROW" and args[FILE] == "short2.bin":
        return list(args[:FILE]) + ["long.bin", args[POLICY]]
    return None


@planted_fault(
    "stray_edit",
    applies_to=["L2-1"],
    cls="uut-state",
    kind="insert",
    mechanism="SET_ROW_ENABLED(2, True) is sent before DUMP_TABLE; row 2 (TB_WORKSTATION_TIME, year 2100) becomes the upcoming row and the dumped table differs from the authored one in one flag",
    fails={
        "L2-1": [
            "last(seqSched.NextRow) == 16",
            "data product file decodes to table T-FIELDS, every field of every row as authored",
        ]
    },
)
def stray_edit(test_id, command_name, args):
    if command_name == S + ".DUMP_TABLE":
        return [(S + ".SET_ROW_ENABLED", [2, True]), (command_name, args)]
    return None


@planted_fault(
    "early_override",
    applies_to=["L2-3"],
    cls="uut-state",
    kind="insert",
    mechanism="SET_TIME_BASE(TB_SC_TIME) is sent in window W1, right after row 1's SET_ROW; row 0 is judged in W1, where the plan asserts the scheduler does nothing",
    fails={
        "L2-3": [
            "event seqSched.SeqRun count == 0",
            "last(seqSched.RowsPending) == 16",
            "last(seqSched.NextRow) == 16",
            "event seqSched.RowLate(row=0) count == 1",
            "last(seqSched.RowsSkipped) == 1",
        ]
    },
)
def early_override(test_id, command_name, args):
    if command_name == S + ".SET_ROW" and int(args[ROW]) == 1:
        return [(command_name, args), (T + ".SET_TIME_BASE", ["TB_SC_TIME"])]
    return None


@planted_fault(
    "cross_instance",
    applies_to=["L2-10"],
    cls="stimulus",
    mechanism="seqSchedB's SET_ROW(0, ...) is sent to seqSched as SET_ROW(1, ...); seqSched runs two rows and seqSchedB none",
    fails={
        "L2-10": [
            "event seqSchedB.SeqRun(row=0, fileName=short2.bin) count == 1",
            "event schedSeq.CS_SequenceLoaded(fileName=short2.bin) count == 0",
            "event schedSeqB.CS_SequenceComplete(fileName=short2.bin) count == 1",
            "event seqSchedB.SeqCompleted(row=0, response=OK) count == 1",
            "event seqSched.SeqRun count == 1",
            "event seqSchedB.SeqRun count == 1",
            "last(seqSched.RowsExecuted) == 1",
            "last(seqSched.RowsPending) == 15",
            "last(seqSchedB.RowsExecuted) == 1",
            "last(seqSchedB.RowsPending) == 14",
        ]
    },
)
def cross_instance(test_id, command_name, args):
    if command_name == B + ".SET_ROW" and int(args[ROW]) == 0:
        return [(S + ".SET_ROW", [1] + list(args[TIME:]))]
    return None


@planted_fault(
    "missing_file_present",
    applies_to=["L5-1"],
    cls="stimulus",
    mechanism="TABLE_FILE_PRM_SET(missing.tbl) is sent with the path of T-BASE; the load the plan expects to fail succeeds and replaces the table, so row 0 is not run",
    fails={
        "L5-1": [
            "event cmdDisp.OpCodeError count == 2",
            "event seqSched.TableLoadFailed(reason=OPEN) count == 1",
            "event seqSched.TableLoaded count == 1",
            "event seqSched.SeqRun(row=0, fileName=short.bin) count == 1",
            "event schedSeq.CS_SequenceComplete(fileName=short.bin) count == 1",
            "last(seqSched.RowsPending) == 15",
            "last(seqSched.RowsExecuted) == 1",
        ]
    },
)
def missing_file_present(test_id, command_name, args):
    if command_name == S + ".TABLE_FILE_PRM_SET" and args[0] == "missing.tbl":
        return [T_BASE_PATH]
    return None
