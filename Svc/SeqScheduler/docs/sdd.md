# Svc::SeqScheduler

## Introduction

`Svc::SeqScheduler` dispatches command sequences from a fixed-size table of rows. Each row
names a sequence file, a time and a time mode: an absolute time in a named time base, an
elapsed time since the scheduler's first run cycle, or an elapsed time since the system clock
switched to a named time base. Rows are held as parameters so a table survives a restart, are
edited by command, and are reported in a data product for ground verification. One instance
drives one sequencer; a deployment may instantiate several schedulers.

## Requirements

| ID | Requirement | Verification |
|---|---|---|
| REQ-SEQSCHED-001 | The scheduler shall hold a table of `SeqSchedulerRows` rows with the fields of Table 1. | Unit Test |
| REQ-SEQSCHED-002 | The scheduler shall load every row from its parameter at initialization. | Unit Test |
| REQ-SEQSCHED-003 | An enabled row whose parameter is absent, invalid or violates Table 6 at initialization shall be disabled with a `WARNING_LO` event. | Unit Test |
| REQ-SEQSCHED-004 | The scheduler shall dispatch an eligible `ABSOLUTE` row on the first run cycle with system time at or past the row time. | Unit Test |
| REQ-SEQSCHED-005 | An `ABSOLUTE` row shall be eligible only while the system time base equals the row time base. | Unit Test |
| REQ-SEQSCHED-006 | The scheduler shall dispatch an eligible `START_RELATIVE` row on the first run cycle with time since start at or past the row offset. | Unit Test |
| REQ-SEQSCHED-007 | Time since start shall be measured from the scheduler's first run cycle. | Unit Test |
| REQ-SEQSCHED-008 | Time since start shall continue to accumulate across a system time base change. | Unit Test |
| REQ-SEQSCHED-009 | The scheduler shall dispatch an eligible `TIME_BASE_RELATIVE` row on the first run cycle with time in the row time base at or past the row offset. | Unit Test |
| REQ-SEQSCHED-010 | A `TIME_BASE_RELATIVE` row shall be eligible only while the system time base equals the row time base. | Unit Test |
| REQ-SEQSCHED-011 | Time in a time base shall be measured from the first run cycle on which that base is reported. | Unit Test |
| REQ-SEQSCHED-012 | Time in a time base shall restart at zero on each entry into that base. | Unit Test |
| REQ-SEQSCHED-013 | A backward step of system time within a base shall not reduce time since start or time in that base. | Unit Test |
| REQ-SEQSCHED-014 | The scheduler shall dispatch a row by sending its sequence file name and empty arguments to the sequencer. | Unit Test |
| REQ-SEQSCHED-015 | The scheduler shall dispatch at most one row per run cycle, lowest eligible index first. | Unit Test |
| REQ-SEQSCHED-016 | A row shall be dispatched at most once until a Table 5 edit command is applied to it or the software restarts. | Unit Test |
| REQ-SEQSCHED-017 | A row first eligible more than `LATE_LIMIT_SECONDS` past its due time shall be skipped with a `WARNING_HI` event. | Unit Test |
| REQ-SEQSCHED-018 | Lateness shall be measured on the first run cycle an enabled `PENDING` row's time condition holds. | Unit Test |
| REQ-SEQSCHED-019 | An enabled `PENDING` `TIME_BASE_RELATIVE` or `ABSOLUTE` row whose base is unseen `BASE_WAIT_TICKS` run cycles after start shall be skipped with a `WARNING_HI` event. | Unit Test |
| REQ-SEQSCHED-020 | A row with `waitPrevious` set shall not dispatch while a scheduler-dispatched sequence is running. | Unit Test |
| REQ-SEQSCHED-021 | A `cancelPrevious` row whose time condition holds shall cancel a running scheduler-dispatched sequence, dispatching after its completion is reported. | Unit Test |
| REQ-SEQSCHED-022 | When both `waitPrevious` and `cancelPrevious` are set, `cancelPrevious` shall take precedence. | Unit Test |
| REQ-SEQSCHED-023 | A dispatched row shall become `COMPLETE`, `FAILED` or `CANCELLED` when the sequencer reports the end of its sequence. | Unit Test |
| REQ-SEQSCHED-024 | The scheduler shall ignore a sequence end reported while no row is dispatched, or an `OK` end reported before the dispatched row's start. | Unit Test |
| REQ-SEQSCHED-025 | A dispatched row whose end is unreported after `DONE_TIMEOUT_TICKS` run cycles shall become `TIMED_OUT` with a `WARNING_HI` event. | Unit Test |
| REQ-SEQSCHED-026 | The scheduler shall apply each Table 5 edit command to the indexed row from the next run cycle. | Unit Test |
| REQ-SEQSCHED-027 | A Table 5 edit command shall not change the row's stored parameter. | Unit Test |
| REQ-SEQSCHED-028 | A row to which a Table 5 edit command is applied shall return to the `PENDING` state. | Unit Test |
| REQ-SEQSCHED-029 | The scheduler shall respond `VALIDATION_ERROR` with a `WARNING_LO` event to a command naming an index outside the table. | Unit Test |
| REQ-SEQSCHED-030 | The scheduler shall respond `VALIDATION_ERROR` with a `WARNING_LO` event to a row edit violating Table 6. | Unit Test |
| REQ-SEQSCHED-031 | The scheduler shall respond `VALIDATION_ERROR` with a `WARNING_LO` event to an edit of a `DISPATCHED` row. | Unit Test |
| REQ-SEQSCHED-032 | `SAVE_TABLE` shall write every row to its parameter. | Unit Test |
| REQ-SEQSCHED-033 | A table saved by command and a saved parameter database shall be restored at the next initialization. | Integration Test |
| REQ-SEQSCHED-034 | `DUMP_TABLE` shall produce a data product containing every row and its Table 2 state. | Unit Test |
| REQ-SEQSCHED-035 | `DUMP_TABLE` shall respond `EXECUTION_ERROR` with a `WARNING_LO` event when no data product buffer is available. | Unit Test |
| REQ-SEQSCHED-036 | The scheduler shall report the Table 7 telemetry channels every run cycle. | Unit Test |
| REQ-SEQSCHED-037 | The scheduler shall emit each Table 8 event at the occurrence it names. | Unit Test |
| REQ-SEQSCHED-038 | A `ROW_n` parameter set at run time shall re-arm the row from the next run cycle, or be left unapplied with a `WARNING_LO` event if it violates Table 6 or the row is `DISPATCHED`. | Unit Test |
| REQ-SEQSCHED-039 | A Table 4 limit parameter set at run time shall take effect from the next run cycle. | Unit Test |
| REQ-SEQSCHED-040 | Two scheduler instances in one deployment shall dispatch and track their rows independently. | Unit Test |
| REQ-SEQSCHED-041 | A row with neither `waitPrevious` nor `cancelPrevious` whose time condition holds while a row is `DISPATCHED` shall become `FAILED` without dispatch, with a `WARNING_HI` event. | Unit Test |

Table 1 — Row fields

| Field | Type | Meaning |
|---|---|---|
| `enabled` | bool | Row is considered for dispatch |
| `mode` | `ABSOLUTE`, `START_RELATIVE`, `TIME_BASE_RELATIVE` | How `timeBase`, `seconds`, `useconds` are interpreted |
| `timeBase` | `TimeBase` | Base of an `ABSOLUTE` time or of a `TIME_BASE_RELATIVE` entry; ignored for `START_RELATIVE` |
| `seconds`, `useconds` | U32, U32 | Absolute time, or offset from start / from entry into `timeBase` |
| `waitPrevious` | bool | Hold until no scheduler-dispatched sequence is running |
| `cancelPrevious` | bool | Cancel a running scheduler-dispatched sequence first |
| `sequence` | string, `SeqSchedulerSequenceNameSize` | Sequence file name passed to the sequencer |

Table 2 — Row states: `PENDING` (not yet dispatched; disabled rows stay here), `DISPATCHED` (sent, end
not yet reported), `COMPLETE` (end reported `OK`), `FAILED` (end reported non-`OK`: rejection or failure),
`CANCELLED` (cancelled by a `cancelPrevious` row), `TIMED_OUT` (no end within `DONE_TIMEOUT_TICKS`),
`SKIPPED_LATE`, `SKIPPED_NO_BASE`. A row is *eligible* when enabled, `PENDING`, its mode's time-base
condition holds, and it is not held by `waitPrevious` or `cancelPrevious`. Any Table 5 edit returns a row
to `PENDING`; a past-due row re-armed this way dispatches on the next cycle (so `ENABLE_ROW` on a finished
row is "run now"). To clear a `DISPATCHED` row whose end never arrives, set `DONE_TIMEOUT_TICKS` non-zero.

Table 3 — Constants (`AcConstants.fpp`): `SeqSchedulerRows` = 8, `SeqSchedulerSequenceNameSize` = 80.

Table 4 — Parameters: `ROW_0` … `ROW_7` (one Table 1 row each, default disabled);
`LATE_LIMIT_SECONDS` (U32, default 10, 0 = never skip); `BASE_WAIT_TICKS` (U32, default 0 = wait
indefinitely); `DONE_TIMEOUT_TICKS` (U32, default 0 = no timeout). Tick limits count `run` cycles (1 s in
Ref). A flight deployment should set both tick limits non-zero; 0 is a test default.

Table 5 — Commands (edit commands marked *)

| Command | Arguments | Effect |
|---|---|---|
| `ENABLE_ROW` * | index | Set `enabled` (a past-due row then dispatches next cycle) |
| `DISABLE_ROW` * | index | Clear `enabled` |
| `SET_ROW_SEQUENCE` * | index, sequence | Replace `sequence` |
| `SET_ROW_TIME` * | index, mode, timeBase, seconds, useconds | Replace the time fields |
| `SET_ROW_FLAGS` * | index, waitPrevious, cancelPrevious | Replace the flags |
| `SAVE_TABLE` | — | `paramSet` every `ROW_n` from the RAM row, then `paramSave` (the autocoded parameter-save command for `ROW_n` alone persists the last loaded or set value, not command edits; mnemonics per the generated dictionary) |
| `DUMP_TABLE` | — | Produce the table data product |

Table 6 — Row validity: `useconds` < 1 000 000; `sequence` not empty; `timeBase` ∉ {`TB_DONT_CARE`,
`TB_NONE`} when `mode` ≠ `START_RELATIVE`.

Table 7 — Telemetry: `NextRow` (lowest index among *enabled* `PENDING` rows, those whose time condition
already holds first, then earliest due time; disabled rows excluded; `SeqSchedulerRows` when none);
`NextMode` (its mode); `NextTime` (its time — `timeBase` = row base — for `ABSOLUTE`, else its offset with
`TB_DONT_CARE`; (0,0) when none); `RowsPending`, `RowsDispatched`, `RowsComplete`, `RowsFailed`
(`FAILED`+`CANCELLED`+`TIMED_OUT`), `RowsSkipped` (both `SKIPPED_*`); `SinceStart`, `SinceBase` (whole
seconds of the two relative clocks, REQ-007/011).

Table 8 — Events

| Event | Severity | Occurrence (arguments) |
|---|---|---|
| `RowDispatched` | `ACTIVITY_HI` | Row sent to the sequencer (index, sequence, time) |
| `RowCompleted` | `ACTIVITY_HI` | End reported `OK` (index) |
| `RowFailed` | `WARNING_HI` | End reported other than `OK`: rejection, failure or cancel — the sequencer's own events give the cause; or `BUSY` for a row due while another is `DISPATCHED` (REQ-041) (index, response) |
| `RowTimedOut` | `WARNING_HI` | REQ-SEQSCHED-025 (index) |
| `RowSkippedLate` | `WARNING_HI` | REQ-SEQSCHED-017 (index, due time, current time) |
| `RowSkippedNoBase` | `WARNING_HI` | REQ-SEQSCHED-019 (index, time base) |
| `RowCancelled` | `ACTIVITY_HI` | Cancel sent for `cancelPrevious` (cancelling index, running index) |
| `RowEdited` | `ACTIVITY_LO` | Table 5 edit command applied (index) |
| `RowRejected` | `WARNING_LO` | REQ-SEQSCHED-003/029/030/031/038 (index, reason: commands → `BAD_INDEX`, `BAD_USECONDS`, `EMPTY_SEQUENCE`, `BAD_TIME_BASE`, `ROW_DISPATCHED`; init → `PARAM_INVALID_INIT`; run-time set → `PARAM_INVALID_SET`, `PARAM_ROW_DISPATCHED`) |
| `TableSaved` | `ACTIVITY_HI` | `SAVE_TABLE` complete |
| `ForeignSeqDone` | `DIAGNOSTIC` | Sequence end with no row dispatched, ignored (response) |
| `UnconfirmedSeqDone` | `WARNING_LO` | `OK` end before the dispatched row's start, ignored (index, response) |
| `QueueFull` | `WARNING_LO` | Command received with the queue full, answered `BUSY` (opcode) |
| `DumpFailed` | `WARNING_LO` | REQ-SEQSCHED-035 |
| `TimeBaseChanged` | `ACTIVITY_HI` | System time base differs from the previous cycle (previous, current) |

## Design

```
                   +-----------------------------------------+
 run : Svc.Sched   |                                         | seqRunOut : Svc.CmdSeqIn
   [sync] -------->|                                         |------>
 seqStartIn :      |            Svc.SeqScheduler             | seqCancelOut : Svc.CmdSeqCancel
 Svc.CmdSeqIn      |              (queued)                   |------>
   [async,drop] -->|                                         | productGetOut / productSendOut
 seqDoneIn :       |   internal: paramUpdated(prmId)         |------>
 Fw.CmdResponse    |                                         |
   [async,drop] -->+-----------------------------------------+
   special ports: command (async, hook), event, text event, time, telemetry, param get/set
```

| Port | Kind | Type | Purpose |
|---|---|---|---|
| `run` | sync input | `Svc.Sched` | Run cycle: drain queue, then evaluate table |
| `seqRunOut` | output | `Svc.CmdSeqIn` | Start a row's sequence |
| `seqCancelOut` | output | `Svc.CmdSeqCancel` | Cancel the running sequence (`cancelPrevious`) |
| `seqStartIn` | async input, drop | `Svc.CmdSeqIn` | Sequencer reports a sequence started (file name) — attributes the row; two schedulers must not share one sequencer |
| `seqDoneIn` | async input, drop | `Fw.CmdResponse` | Sequence end: OK, rejection, failure or cancel (no identity) |
| `paramUpdated` | internal, drop | `FwPrmIdType` | Re-queues the autocoded `parameterUpdated` hook onto the run thread; suppressed while `SAVE_TABLE` writes |
| `productGetOut`, `productSendOut` | DP | `Fw.DpGet`, `Fw.DpSend` | Table data product, synchronous get |

The component is **queued**: commands, `seqStartIn`, `seqDoneIn` and `paramUpdated` are messages
drained by the `run` handler, so every table change happens on the rate-group thread and the table
needs no lock. Commands therefore complete one run period late. Queue-full policy: commands carry
`hook` and are answered `BUSY` with `QueueFull`; the two sequencer ports `drop` (a dropped end is
bounded by `DONE_TIMEOUT_TICKS`). Commands, parameters, events and telemetry are those of
Tables 5, 4, 8 and 7; the FPP (`SeqScheduler.fpp`) is the source of truth. Types:

| Type | Kind | Content |
|---|---|---|
| `Svc.SeqSchedulerMode` | enum U8 | `ABSOLUTE`, `START_RELATIVE`, `TIME_BASE_RELATIVE` |
| `Svc.SeqSchedulerRowState` | enum U8 | Table 2 states |
| `Svc.SeqSchedulerRow` | struct | Table 1 fields; default disabled, `START_RELATIVE`, `TB_NONE` |
| `Svc.SeqSchedulerRowRecord` | struct | `index`, `rowState`, `row` — one DP record (`RowRecord` id 0) |
| `Svc.SeqSchedulerReject` | enum U8 | `RowRejected` reasons (Table 8) |

### Configure

```fpp
# default/config/AcConstants.fpp
constant SeqSchedulerRows = 8                 # rows per table (ROW_0 .. ROW_7 parameters)
constant SeqSchedulerSequenceNameSize = 80    # sequence file name length
# topology instances.fpp — queue depth >= 2 * SeqSchedulerRows + 4
#   (a full-table PRM_SET/PRM_SAVE burst, plus a sequence start, an end and a spare)
instance seqScheduler: Svc.SeqScheduler base id 0x10026000 queue size 32
```

`run` is driven from a rate group (1 Hz in Ref) and the topology calls `configure(period)` with that
period (`Fw::TimeInterval`); the two relative clocks count run cycles and are compared as
`cycles × period`, so tick parameters and relative offsets share one unit. The DP container `Table`
(id 0, FPP default priority 10, overridable by the implementation) holds `SeqSchedulerRows`
`RowRecord`s; DP wiring is optional — with `productGetOut` unconnected `DUMP_TABLE` answers
`EXECUTION_ERROR` + `DumpFailed`. Wire `seqStartIn` from the sequencer's `seqStartOut` and
`seqDoneIn` from its `seqDone`; the time source must report the bases the rows name. Budget: 11
`PrmDb` entries per instance (`PRMDB_NUM_DB_ENTRIES` = 25 by default — raise it for two instances);
`SeqSchedulerSequenceNameSize` = 80 bounds the row's `string` within `FW_PARAM_BUFFER_MAX_SIZE`, so a
sequencer path longer than 80 characters cannot be scheduled (rejected at `SET_ROW_SEQUENCE`
deserialization). No dynamic allocation; the table walk is bounded by `SeqSchedulerRows`. No query
port is provided for upcoming times (Q9): `NextRow`/`NextMode`/`NextTime` telemetry is the interface.

### Behavior

Each `run` cycle the scheduler drains its queue, reads `getTime()` for the current base and for
`ABSOLUTE` comparisons, and advances its two relative clocks by one cycle (`SinceStart` = cycles since
the first `run`, REQ-007; `SinceBase` = cycles since the first `run` reporting the current base, reset
with `TimeBaseChanged`, REQ-011/012). Cycle counting makes the relative clocks immune to steps of the
system clock within a base (REQ-013) and to the incomparable delta across a base change; `ABSOLUTE`
rows follow the system clock by definition. It then walks rows 0..N-1 once. A row whose time condition first holds this cycle is stamped; if it is more than
`LATE_LIMIT_SECONDS` past due it becomes `SKIPPED_LATE` (REQ-017/018). The lowest eligible row —
enabled, `PENDING`, condition held, not held by `waitPrevious` while a row is `DISPATCHED` — is sent
on `seqRunOut` with empty `SeqArgs` and becomes `DISPATCHED` (one per cycle, REQ-014/015). The sequencer
runs one sequence at a time, so a due row while any row is `DISPATCHED` is resolved by its flags in this
order: `cancelPrevious` cancels the running row and dispatches after its end (REQ-021/022); otherwise
`waitPrevious` holds it (REQ-020); a row with neither flag is not sent and becomes `FAILED` with
`RowFailed(index, BUSY)` (REQ-041).

Sequence ends carry no identity, so attribution uses the start report: after a dispatch the row is
*unconfirmed* until a `seqStartIn` whose file name equals the row's `sequence` arrives. While
unconfirmed, a non-`OK` `seqDoneIn` is taken as the sequencer's rejection of our run (busy, `MANUAL`
mode, file not loadable — all answered at once with `EXECUTION_ERROR` and no start) → `FAILED`,
`RowFailed`; an `OK` end is a foreign sequence finishing → ignored with `UnconfirmedSeqDone`
(REQ-024). An end with no row dispatched is ignored with `ForeignSeqDone`. Once confirmed, the next
`seqDoneIn` closes the row: `COMPLETE` on `OK` (`RowCompleted`), `FAILED` otherwise (`RowFailed`);
`DONE_TIMEOUT_TICKS` closes a row whose end never comes (`TIMED_OUT`). The two relative clocks are
`U64` cycle counters (`SinceStart`/`SinceBase` saturate at `U32` max); `configure` asserts a non-zero
period and must precede the first `run`, which asserts otherwise. A `cancelPrevious` row whose condition holds while a row is `DISPATCHED`
sends `seqCancelOut` once (`RowCancelled`), stays `PENDING`, and dispatches on the cycle after the
running row leaves `DISPATCHED` — which it does as `CANCELLED` on the cancel's `seqDoneIn`
(`RowFailed`) or as `TIMED_OUT` (REQ-021).

Edit commands are validated against Table 6 and the row state, applied to RAM only, and re-arm the
row (REQ-026..031). The autocoded `parameterUpdated(id)` only invokes `paramUpdated`; its handler
(on the run thread) reads the parameter and applies the same validation — an invalid value or a
`DISPATCHED` target is left unapplied with `RowRejected(PARAM_INVALID_SET / PARAM_ROW_DISPATCHED)`, and the
stored parameter then differs from the RAM row until the next valid edit or `SAVE_TABLE`
(REQ-038); a notification dropped on queue full leaves the same tolerated state. `SAVE_TABLE` writes
`paramSet`/`paramSave` for every row with a suppression flag set, and `parameterUpdated` returns without
queueing while the flag is set — the component-side `paramSet_ROW_n` also fires the hook, and without
the flag a save would re-arm and re-run every finished past-due row. The flag is a `std::atomic<bool>`
(a ground `PRM_SET` reads it on the dispatcher thread); a ground set landing during `SAVE_TABLE` is
stored but not notified, and the save then overwrites it from RAM — one save duration per boot, accepted.
The same flag is set around `loadParameters()`: the autocoded `parameterLoaded()` fires the hook for every
loaded parameter, which would otherwise enqueue eleven notifications that the first run replays as
run-time sets (duplicate `PARAM_INVALID_*` reports, spurious re-arms). Residual aliasing (accepted): a
ground-started sequence with the same file name as an unconfirmed row confirms that row. `DUMP_TABLE` gets one
`Table` container synchronously, serializes every row and sends it inside the handler;
no buffer → `EXECUTION_ERROR` with `DumpFailed`.

```mermaid
sequenceDiagram
  participant RG as rateGroup (run)
  participant S as SeqScheduler
  participant CS as CmdSequencer
  RG->>S: run(n) — drain queue, update clocks
  S->>CS: seqRunOut("a.bin") row 2 → DISPATCHED (unconfirmed)
  CS-->>S: seqStartIn("a.bin")  [queued]
  RG->>S: run(n+1) — row 2 confirmed; row 3 waitPrevious: held
  CS-->>S: seqDoneIn(OK)  [queued]
  RG->>S: run(n+2) — row 2 COMPLETE; row 3 eligible → seqRunOut("b.bin")
  RG->>S: run(n+k) — row 5 cancelPrevious due → seqCancelOut; RowCancelled
  CS-->>S: seqDoneIn(EXECUTION_ERROR) [queued]
  RG->>S: run(n+k+1) — row 3 CANCELLED (RowFailed); row 5 → seqRunOut("c.bin")
```

Out-of-scope failure modes

| Failure | Owner |
|---|---|
| Sequence file missing or malformed | `Svc.CmdSequencer`: `seqDone(EXECUTION_ERROR)` at once, no start → row `FAILED`, no retry |
| Sequencer busy with a ground-started sequence or in `MANUAL` mode | `Svc.CmdSequencer`: `CS_InvalidMode` + `seqDone(EXECUTION_ERROR)` → row `FAILED`, no retry |
| Sequencer never reports an end | bounded by `DONE_TIMEOUT_TICKS` (0 = never; test default) |
| Foreign sequence with the row's file name started while the row is unconfirmed | attributed to the row (identity limit of `Svc.CmdSeqIn`/`Fw.CmdResponse`); do not share a sequencer between schedulers |
| `seqStartIn` dropped on queue full | row ends `TIMED_OUT` although the sequence ran (sizing rule in Configure) |
| Parameter database not saved to disk | `Svc.PrmDb` `PRM_SAVE_FILE` |
| Time source reports a wrong base or non-monotonic time | time component |

## Tests

<!-- fth:tests:begin -->
_Generated by `fth render-sdd-tests` from the plan — do not edit by hand; the package renderer fails when this table and the plan disagree._

| Test | Purpose | Requirements |
|---|---|---|
| L0-1 | Confirms the running Ref image is the manifest's build (Svc.Version payloads equal the pinned describe strings). | — |
| L0-2 | Proves the marker round-trips and the scheduler starts with eight pending, no due row and no override. | REQ-SEQSCHED-036, REQ-TBO-005 |
| L2-1 | Proves a START_RELATIVE row dispatches its sequence once when time since start reaches the offset and is marked complete. | REQ-SEQSCHED-006, REQ-SEQSCHED-007, REQ-SEQSCHED-014, REQ-SEQSCHED-016, REQ-SEQSCHED-023, REQ-SEQSCHED-026, REQ-SEQSCHED-028, REQ-SEQSCHED-036, REQ-SEQSCHED-037 |
| L2-2 | Proves an ABSOLUTE row dispatches only while the system base equals its base, at its absolute time and not before. | REQ-SEQSCHED-004, REQ-SEQSCHED-005, REQ-SEQSCHED-014, REQ-SEQSCHED-023, REQ-SEQSCHED-026, REQ-SEQSCHED-037 |
| L2-3 | Proves a TIME_BASE_RELATIVE row waits for its base and dispatches the offset after the base is first reported. | REQ-SEQSCHED-009, REQ-SEQSCHED-010, REQ-SEQSCHED-011, REQ-SEQSCHED-014, REQ-SEQSCHED-037, REQ-TBO-002, REQ-TBO-003, REQ-TBO-004, REQ-TBO-005 |
| L2-4 | Proves two rows due together dispatch lowest index first and a waitPrevious row waits for the running sequence to complete. | REQ-SEQSCHED-015, REQ-SEQSCHED-020, REQ-SEQSCHED-023, REQ-SEQSCHED-037 |
| L2-5 | Proves a cancelPrevious row cancels the running scheduler sequence and dispatches after the cancelled completion is reported. | REQ-SEQSCHED-021, REQ-SEQSCHED-023, REQ-SEQSCHED-037 |
| L2-6 | Proves DUMP_TABLE writes a data product holding every row with its state. | REQ-SEQSCHED-034, REQ-SEQSCHED-037 |
| L2-7 | Proves a second ABSOLUTE row in the current base dispatches at its absolute time and completes. | REQ-SEQSCHED-004, REQ-SEQSCHED-005, REQ-SEQSCHED-014, REQ-SEQSCHED-023 |
| L2-8 | Proves a TIME_BASE_RELATIVE row for TB_PROC_TIME waits for that base and dispatches the offset after it is first reported. | REQ-SEQSCHED-009, REQ-SEQSCHED-010, REQ-SEQSCHED-011, REQ-SEQSCHED-014 |
| L3-1 | Proves a saved table survives a restart and a START_RELATIVE row dispatches the offset after the new start. | REQ-SEQSCHED-002, REQ-SEQSCHED-006, REQ-SEQSCHED-007, REQ-SEQSCHED-016, REQ-SEQSCHED-027, REQ-SEQSCHED-032, REQ-SEQSCHED-033 |
| L4-1 | Proves SinceStart keeps accumulating and SinceBase restarts when the base changes, both advancing one second per run cycle over a 60 s strip. | REQ-SEQSCHED-007, REQ-SEQSCHED-008, REQ-SEQSCHED-012, REQ-SEQSCHED-036, REQ-TBO-002, REQ-TBO-003, REQ-TBO-004 |
| L5-1 | Proves a row first due more than LATE_LIMIT_SECONDS ago is skipped with a warning and never dispatched. | REQ-SEQSCHED-017, REQ-SEQSCHED-018, REQ-SEQSCHED-037 |
| L5-2 | Proves an unreported end times out after DONE_TIMEOUT_TICKS and the late sequencer report is ignored as foreign. | REQ-SEQSCHED-024, REQ-SEQSCHED-025, REQ-SEQSCHED-039, REQ-SEQSCHED-037 |
| L5-3 | Proves a row for a base not reported within BASE_WAIT_TICKS is skipped with a warning. | REQ-SEQSCHED-019, REQ-SEQSCHED-039, REQ-SEQSCHED-037 |
| L5-4 | Proves a START_RELATIVE row whose offset is already more than LATE_LIMIT_SECONDS in the past is skipped, never dispatched. | REQ-SEQSCHED-017, REQ-SEQSCHED-018 |
| L6-1 | Proves every Table 6 violation and the override of TB_DONT_CARE are rejected with a warning and change nothing. | REQ-SEQSCHED-029, REQ-SEQSCHED-030, REQ-TBO-006, REQ-SEQSCHED-037 |
| L6-2 | Proves a running row cannot be edited while other rows can, and that bad-index, Table 6 and TB_DONT_CARE requests are rejected. | REQ-SEQSCHED-031, REQ-SEQSCHED-028, REQ-SEQSCHED-029, REQ-SEQSCHED-030, REQ-TBO-006, REQ-SEQSCHED-037 |
| L7-1 | Leaves every row disabled and pending, no override, and a table dump the auditor decodes independently. | REQ-SEQSCHED-028, REQ-SEQSCHED-032, REQ-SEQSCHED-034 |
<!-- fth:tests:end -->

## Unit-test list

| Test | Description | Requirements |
|---|---|---|
| `TableLoad` | Eight rows load from `ROW_n` at init; an absent or invalid enabled row is disabled with `RowRejected(PARAM_INVALID_INIT)`; disabled rows are not validated. | REQ-SEQSCHED-001, -002, -003 |
| `AbsoluteDispatch` | `ABSOLUTE` row dispatches on the first cycle at or past its time in its base. | REQ-SEQSCHED-004, -014 |
| `AbsoluteBaseMismatch` | `ABSOLUTE` row is not eligible while the system base differs from the row base. | REQ-SEQSCHED-005 |
| `StartRelativeDispatch` | `START_RELATIVE` row dispatches when time since the first run cycle reaches the offset. | REQ-SEQSCHED-006, -007 |
| `StartRelativeAcrossBase` | Time since start keeps accumulating through a base change. | REQ-SEQSCHED-008 |
| `BaseRelativeDispatch` | `TIME_BASE_RELATIVE` row dispatches the offset after its base is first seen. | REQ-SEQSCHED-009, -010, -011 |
| `BaseRelativeReentry` | Time in a base restarts at zero on re-entry. | REQ-SEQSCHED-012 |
| `BackwardStep` | A backward system-time step reduces neither relative clock. | REQ-SEQSCHED-013 |
| `OnePerCycleOrder` | Two due rows dispatch on consecutive cycles, lowest index first. | REQ-SEQSCHED-015 |
| `DispatchOnce` | A dispatched row is not dispatched again until edited. | REQ-SEQSCHED-016 |
| `LateSkip` | Row first due beyond `LATE_LIMIT_SECONDS` is `SKIPPED_LATE` with `RowSkippedLate`; a held row is not late. | REQ-SEQSCHED-017, -018 |
| `BaseNeverSeen` | Row whose base is unseen after `BASE_WAIT_TICKS` is `SKIPPED_NO_BASE` with `RowSkippedNoBase`; 0 waits forever. | REQ-SEQSCHED-019 |
| `WaitPrevious` | `waitPrevious` row holds until the outstanding row completes. | REQ-SEQSCHED-020 |
| `CancelPrevious` | `cancelPrevious` row cancels the outstanding row and dispatches after its completion. | REQ-SEQSCHED-021 |
| `CancelPrecedence` | Both flags set behaves as `cancelPrevious`. | REQ-SEQSCHED-022 |
| `CompletionOkFail` | After the start is confirmed, `seqDoneIn` OK/error marks the row `COMPLETE`/`FAILED`; a cancelled row ends `CANCELLED`. | REQ-SEQSCHED-023 |
| `StrayCompletion` | End with no row dispatched → `ForeignSeqDone`; `OK` end before the start → `UnconfirmedSeqDone`; error before the start → `FAILED`. | REQ-SEQSCHED-024 |
| `DoneTimeout` | Unreported end times out after `DONE_TIMEOUT_TICKS`: row `TIMED_OUT` with `RowTimedOut`. | REQ-SEQSCHED-025 |
| `BusyFail` | Row with neither flag due while another is `DISPATCHED` becomes `FAILED` with `RowFailed(BUSY)`, nothing sent; a `cancelPrevious` row in the same state cancels instead. | REQ-SEQSCHED-021, -041 |
| `ForeignEnd` | Error end before the start → `FAILED`; OK end before the start → `UnconfirmedSeqDone`; end with nothing dispatched → `ForeignSeqDone`. | REQ-SEQSCHED-023, -024 |
| `ParamUnapplied` | Run-time `ROW_n` failing Table 6 or targeting a `DISPATCHED` row is left unapplied with `RowRejected(PARAM_INVALID_SET / PARAM_ROW_DISPATCHED)`. | REQ-SEQSCHED-038 |
| `NeverSkip` | `LATE_LIMIT_SECONDS` = 0 dispatches an arbitrarily late row. | REQ-SEQSCHED-017 |
| `EditCommands` | Each Table 5 edit applies next cycle, re-arms the row, leaves the parameter unchanged. | REQ-SEQSCHED-026, -027, -028 |
| `EditRejects` | Bad index, Table 6 violations (incl. `TB_NONE`/`TB_DONT_CARE` on timed modes) and edits of a `DISPATCHED` row return `VALIDATION_ERROR` with `RowRejected`. | REQ-SEQSCHED-029, -030, -031 |
| `SaveTable` | `SAVE_TABLE` writes every row with `paramSet`/`paramSave`. | REQ-SEQSCHED-032 |
| `DumpTable` | `DUMP_TABLE` fills one `Table` container with eight `RowRecord`s carrying every field and each of the eight states. | REQ-SEQSCHED-034 |
| `DumpNoBuffer` | `DUMP_TABLE` with no buffer returns `EXECUTION_ERROR` with `DumpFailed`. | REQ-SEQSCHED-035 |
| `Telemetry` | All Table 7 channels (incl. `RowsFailed`, `NextMode`, `SinceStart`, `SinceBase`) are written every run cycle with the expected values. | REQ-SEQSCHED-036 |
| `StartAttribution` | `seqStartIn` with the row's name confirms it; a same-name foreign start is attributed; a dropped start ends in `TIMED_OUT`. | REQ-SEQSCHED-023, -024 |
| `QueueFullHook` | A command arriving with the queue full is answered `BUSY` with `QueueFull`. | REQ-SEQSCHED-037 |
| `SaveNoRearm` | `SAVE_TABLE` does not re-arm or re-dispatch any finished row. | REQ-SEQSCHED-016, -032 |
| `ConfigurePeriod` | Relative clocks advance `period` per cycle; `configure` with a zero period or a `run` before `configure` asserts. | REQ-SEQSCHED-007, -011 |
| `Events` | Every Table 8 event fires at its named occurrence (checked across the tests above). | REQ-SEQSCHED-037 |
| `RuntimeParams` | `ROW_n` set at run time re-arms next cycle; an invalid or `DISPATCHED` target is left unapplied with `RowRejected`; limits take effect next cycle. | REQ-SEQSCHED-038, -039 |
| `TwoInstances` | Two testers with independent tables dispatch and track rows independently. | REQ-SEQSCHED-040 |
| `Persistence` | Integration test (L3-1); no unit test. | REQ-SEQSCHED-033 |
