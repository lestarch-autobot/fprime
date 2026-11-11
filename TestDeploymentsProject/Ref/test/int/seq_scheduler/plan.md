# SEQSCHED — Svc.SeqScheduler on Ref (TestDeploymentsProject/Ref) — L0: desk-checked; bench confirmation pending

## 0 Scope
- UUT: `Svc.SeqScheduler` (lestarch-autobot/fprime `seq-scheduler`), SHA recorded in the manifest at stage 2; constants `SeqSchedulerRows = 8`, `SeqSchedulerSequenceNameSize = 80`.
- Deployment: `TestDeploymentsProject/Ref` **system deployment** plus `seqScheduler: Svc.SeqScheduler` on `rateGroup1Comp` slot 8 (1 Hz) wired to `cmdSeq` and the DataProducts subtopology, and the harness component `timeBaseOverride: Ref.TimeBaseOverride` between the topology's `time connections` and `posixTime` (delta in `Ref/docs/seq-scheduler-topology.md`).
- Bench: native Linux host (no board, no shared bench); Ref and the GDS run on the same host; FSW time = host UNIX time in `TB_WORKSTATION_TIME`.
- Ground system: fprime-gds (harness skill default) through the `fth` pytest plugin; dictionary from the manifest build.
- Stimulus sequences: `seqs/sched_a.seq` (NO_OP_STRING "SEQ_A"), `sched_b.seq` ("SEQ_B"), `sched_long.seq` ("SEQ_L_START", R00:00:08 "SEQ_L_END"), compiled by `fprime-seqgen` (time base TB_DONT_CARE) into Ref's run directory before the run (entry gate).
- Out of scope: sequence file content/validity (`Svc.CmdSequencer`), parameter file integrity (`Svc.PrmDb`), DP buffer exhaustion (REQ-035, UT), two scheduler instances (REQ-040, UT).

## 1 Entry gates
- Stage-1 close: `fprime-util check` on `Svc/SeqScheduler` and `Ref/TimeBaseOverride` passes with every row of the SDD unit-test lists implemented.
- Image: Ref built from the manifest SHAs with `Svc.Version` enabled (CdhCore subtopology); `version.VERSION ALL` payloads are the manifest's describe strings (L0-1).
- Marker round-trip: `CdhCore.cmdDisp.CMD_NO_OP_STRING("<id>")` echoed as `NoOpStringReceived(message=<id>)` once (L0-2); the event text `Received a NO-OP string=<id>` is the marker regex. Run-start markers use the `RFR <run> M/S <sha8>` string form.
- Downlink budget: ≤ 20 events/s over the Ref TCP link; the plan's densest window (L2-4) emits < 20 events total.
- Bench recipe: `bench.json` names the Ref binary, run directory (sequences and `/tmp/PrmDb.dat` beneath it), the GDS start command and a `reset` template that restarts the Ref process (L3-1).
- Parameter file: deleted before the run so L0-2 sees default rows.
- Run-level parameter: L0-2 sets `DONE_TIMEOUT_TICKS` = 30 before any dispatch; every later test assumes it.

## 2 Test levels
L0 provenance/feasibility · L2 functional · L3 persistence/reset · L4 endurance/performance · L5 fault paths · L6 negative controls · L7 final state for independent verification.

## 3 Test entries

Window rule: rows from the **last** `CdhCore.cmdDisp.NoOpStringReceived(id=START)` row that precedes the first `CdhCore.cmdDisp.NoOpStringReceived(id=END)` row following it, through that END row plus the END `CdhCore.cmdDisp.CMD_NO_OP_STRING` command's own `cmdDisp` rows (at most two), inclusive; ordering key `t = epoch·1e6 + fsw`, archive index on ties. Channel rows are those with `t` between the first and last window event. `commands_sent` = commands from the matching START sending (its last sending if re-sent) through the END sending, inclusive.

Standard `complete` block (STD): both markers present; commands_sent == count(event CdhCore.cmdDisp.OpCodeDispatched) - count(sequence-issued NO-OPs) (check function commands_sent_minus_seq); sched samples >= 2.
Standard absence block (ABS): event CdhCore.cmdDisp.OpCodeError count == 0; event seqScheduler.DumpFailed count == 0; event cmdSeq.CS_FileNotFound count == 0; event cmdSeq.CS_InvalidMode count == 0; event cmdSeq.CS_SequenceTimeout count == 0; event seqScheduler.QueueFull count == 0.
Standard absence block (ABS-REJ): event seqScheduler.DumpFailed count == 0; event cmdSeq.CS_FileNotFound count == 0; event cmdSeq.CS_InvalidMode count == 0; event cmdSeq.CS_SequenceTimeout count == 0; event seqScheduler.QueueFull count == 0.
Packets (named channel sets for `<packet> samples >= n`): sched = seqScheduler.NextRow, seqScheduler.RowsPending, seqScheduler.RowsDispatched, seqScheduler.RowsComplete, seqScheduler.RowsSkipped, seqScheduler.SinceStart, seqScheduler.SinceBase, timeBaseOverride.OverrideTimeBase, seqScheduler.RowsFailed, seqScheduler.NextMode.

ID:        L0-1
Title:     Image identity
Purpose:   Confirms the running Ref image is the manifest's build (Svc.Version payloads equal the pinned describe strings).
Covers:    —
Setup:     Ref started from the manifest build in its run directory; GDS attached; dictionary from the same build.
Procedure: MARK(10); CdhCore.version.VERSION(ALL); wait 2 s; MARK(11).
Window:    Marker(id=10) … Marker(id=11)
Pass rule: named checks declared in the integration test (test_L0_1).
Evidence:  EVRs FrameworkVersion / ProjectVersion / LibraryVersions beside manifest.json repos[].describe.
Planted faults:none

ID:        L0-2
Title:     Marker round-trip and idle table
Purpose:   Proves the marker round-trips and the scheduler starts with eight pending, no due row and no override.
Covers:    REQ-SEQSCHED-036, REQ-TBO-005
Setup:     Fresh start; parameter file absent (default rows: all disabled). Sets the run-level DONE_TIMEOUT_TICKS = 30 (SDD: 0 is a test default) so any stuck row closes within 30 s.
Procedure: MARK(12); seqScheduler.DONE_TIMEOUT_TICKS_PRM_SET(30); bracket (2 sched samples); MARK(13).
Window:    Marker(id=12) … Marker(id=13)
Pass rule: named checks declared in the integration test (test_L0_2).
Evidence:  Telemetry strip RowsPending/NextRow/OverrideTimeBase over the window; no scheduler EVR.
Planted faults:none

ID:        L2-1
Title:     Start-relative dispatch
Purpose:   Proves a START_RELATIVE row dispatches its sequence once when time since start reaches the offset and is marked complete.
Covers:    REQ-SEQSCHED-006, REQ-SEQSCHED-007, REQ-SEQSCHED-014, REQ-SEQSCHED-016, REQ-SEQSCHED-023, REQ-SEQSCHED-026, REQ-SEQSCHED-028, REQ-SEQSCHED-036, REQ-SEQSCHED-037
Setup:     Row 0 disabled; seqs/sched_a.bin present (compiled by fprime-seqgen from sched_a.seq, TB_DONT_CARE). S = last(seqScheduler.SinceStart) read before the window.
Procedure: MARK(20); seqScheduler.SET_ROW_SEQUENCE(0, "seqs/sched_a.bin"); seqScheduler.SET_ROW_TIME(0, START_RELATIVE, TB_NONE, S+5, 0); seqScheduler.ENABLE_ROW(0); wait 12 s; bracket (2 sched samples); MARK(21).
Window:    Marker(id=20) … Marker(id=21)
Pass rule: named checks declared in the integration test (test_L2_1). metrics: t(event seqScheduler.RowDispatched(index=0)) - t(seqScheduler.ENABLE_ROW sent) <= 8 s (offset 5 s + 1 s run period + 1 s SinceStart read granularity + 1 s margin).
Evidence:  EVR sequence RowEdited×3 → RowDispatched(0) → cmdDisp NoOpStringReceived(SEQ_A) → CS_SequenceComplete → RowCompleted(0).
Planted faults:wrong_sequence_name

ID:        L2-2
Title:     Absolute dispatch
Purpose:   Proves an ABSOLUTE row dispatches only while the system base equals its base, at its absolute time and not before.
Covers:    REQ-SEQSCHED-004, REQ-SEQSCHED-005, REQ-SEQSCHED-014, REQ-SEQSCHED-023, REQ-SEQSCHED-026, REQ-SEQSCHED-037
Setup:     Row 1 disabled; no override (Ref base TB_WORKSTATION_TIME); N = host UNIX seconds at send.
Procedure: MARK(22); seqScheduler.SET_ROW_SEQUENCE(1, "seqs/sched_b.bin"); seqScheduler.SET_ROW_TIME(1, ABSOLUTE, TB_SC_TIME, N+4, 0); seqScheduler.ENABLE_ROW(1); wait 6 s; MARK(23); timeBaseOverride.SET_TIME_BASE(TB_SC_TIME); wait 6 s; timeBaseOverride.CLEAR_OVERRIDE; bracket (2 sched samples); MARK(33).
Window:    W1 = Marker(id=22) … Marker(id=23); W2 = Marker(id=23) … Marker(id=33)
Pass rule: named checks declared in the integration test (test_L2_2). metrics: t(event seqScheduler.RowDispatched(index=1)) - t(timeBaseOverride.SET_TIME_BASE sent) <= 3 s.
Evidence:  W1: no dispatch for 6 s while the base mismatches; W2: OverrideChanged → RowDispatched(1) within 3 s → SEQ_B → RowCompleted(1).
Planted faults:late_absolute

ID:        L2-3
Title:     Time-base-relative dispatch
Purpose:   Proves a TIME_BASE_RELATIVE row waits for its base and dispatches the offset after the base is first reported.
Covers:    REQ-SEQSCHED-009, REQ-SEQSCHED-010, REQ-SEQSCHED-011, REQ-SEQSCHED-014, REQ-SEQSCHED-037, REQ-TBO-002, REQ-TBO-003, REQ-TBO-004, REQ-TBO-005
Setup:     Row 2 disabled; no override; Ref base TB_WORKSTATION_TIME.
Procedure: MARK(24); seqScheduler.SET_ROW_SEQUENCE(2, "seqs/sched_a.bin"); seqScheduler.SET_ROW_TIME(2, TIME_BASE_RELATIVE, TB_SC_TIME, 5, 0); seqScheduler.ENABLE_ROW(2); wait 8 s; MARK(25); timeBaseOverride.SET_TIME_BASE(TB_SC_TIME); wait 10 s; timeBaseOverride.CLEAR_OVERRIDE; bracket (2 sched samples); MARK(26).
Window:    W1 = Marker(id=24) … Marker(id=25); W2 = Marker(id=25) … Marker(id=26)
Pass rule: named checks declared in the integration test (test_L2_3). metrics: t(event seqScheduler.RowDispatched(index=2)) - t(timeBaseOverride.SET_TIME_BASE sent) <= 8 s (offset 5 s + 1 s to observe the base + 1 s run period + 1 s margin).
Evidence:  W1: no dispatch for 8 s while the base is absent; W2: OverrideChanged → TimeBaseChanged → RowDispatched(2) ≈5 s later → restore.
Planted faults:disturb_disable

ID:        L2-4
Title:     waitPrevious and lowest-index order
Purpose:   Proves two rows due together dispatch lowest index first and a waitPrevious row waits for the running sequence to complete.
Covers:    REQ-SEQSCHED-015, REQ-SEQSCHED-020, REQ-SEQSCHED-023, REQ-SEQSCHED-037
Setup:     Rows 3 and 4 disabled; seqs/sched_long.bin (8 s) and sched_a.bin present; S = last(SinceStart).
Procedure: MARK(27); seqScheduler.SET_ROW_SEQUENCE(3, "seqs/sched_long.bin"); seqScheduler.SET_ROW_TIME(3, START_RELATIVE, TB_NONE, S+4, 0); seqScheduler.SET_ROW_SEQUENCE(4, "seqs/sched_a.bin"); seqScheduler.SET_ROW_TIME(4, START_RELATIVE, TB_NONE, S+4, 0); seqScheduler.SET_ROW_FLAGS(4, true, false); seqScheduler.ENABLE_ROW(3); seqScheduler.ENABLE_ROW(4); wait 20 s; bracket (2 sched samples); MARK(28).
Window:    Marker(id=27) … Marker(id=28)
Pass rule: named checks declared in the integration test (test_L2_4). metrics: t(event seqScheduler.RowDispatched(index=4)) - t(seqScheduler.ENABLE_ROW sent) <= 18 s.
Evidence:  EVR order: RowDispatched(3) → SEQ_L_START → SEQ_L_END → RowCompleted(3) → RowDispatched(4) → SEQ_A → RowCompleted(4).
Planted faults:drop_wait_flag

ID:        L2-5
Title:     cancelPrevious
Purpose:   Proves a cancelPrevious row cancels the running scheduler sequence and dispatches after the cancelled completion is reported.
Covers:    REQ-SEQSCHED-021, REQ-SEQSCHED-023, REQ-SEQSCHED-037
Setup:     Rows 5 and 6 disabled; S = last(SinceStart).
Procedure: MARK(29); seqScheduler.SET_ROW_SEQUENCE(5, "seqs/sched_long.bin"); seqScheduler.SET_ROW_TIME(5, START_RELATIVE, TB_NONE, S+3, 0); seqScheduler.SET_ROW_SEQUENCE(6, "seqs/sched_b.bin"); seqScheduler.SET_ROW_TIME(6, START_RELATIVE, TB_NONE, S+6, 0); seqScheduler.SET_ROW_FLAGS(6, false, true); seqScheduler.ENABLE_ROW(5); seqScheduler.ENABLE_ROW(6); wait 15 s; bracket (2 sched samples); MARK(30).
Window:    Marker(id=29) … Marker(id=30)
Pass rule: named checks declared in the integration test (test_L2_5). metrics: t(event seqScheduler.RowDispatched(index=6)) - t(event seqScheduler.RowCancelled(index=6, running=5)) <= 1 s.
Evidence:  EVR order: RowDispatched(5) → SEQ_L_START → RowCancelled(6,5) → CS_SequenceCanceled → RowFailed(5, EXECUTION_ERROR; row state CANCELLED) → RowDispatched(6) → SEQ_B; SEQ_L_END absent.
Planted faults:no_cancel_flag

ID:        L2-6
Title:     Table dump
Purpose:   Proves DUMP_TABLE writes a data product holding every row with its state.
Covers:    REQ-SEQSCHED-034, REQ-SEQSCHED-037
Setup:     Rows as left by L2-1..L2-5: 0 COMPLETE (sched_a, START_RELATIVE), 1 COMPLETE (sched_b, ABSOLUTE), 2 COMPLETE (sched_a, TIME_BASE_RELATIVE TB_SC_TIME), 3 COMPLETE (sched_long), 4 COMPLETE (sched_a, waitPrevious), 5 CANCELLED (sched_long), 6 COMPLETE (sched_b, cancelPrevious), 7 PENDING disabled (default); DataProducts subtopology running.
Procedure: MARK(31); seqScheduler.DUMP_TABLE; wait 3 s; MARK(32).
Window:    Marker(id=31) … Marker(id=32)
Pass rule: named checks declared in the integration test (test_L2_6).
Evidence:  EVR FileWritten(bytes, file); the file itself (independent of the scheduler's own reporting) decoded by the check function.
Planted faults:none

ID:        L2-7
Title:     Second absolute row
Purpose:   Proves a second ABSOLUTE row in the current base dispatches at its absolute time and completes.
Covers:    REQ-SEQSCHED-004, REQ-SEQSCHED-005, REQ-SEQSCHED-014, REQ-SEQSCHED-023
Setup:     Row 7 disabled; no override; N = host UNIX seconds at send.
Procedure: MARK(34); seqScheduler.SET_ROW_SEQUENCE(7, "seqs/sched_a.bin"); seqScheduler.SET_ROW_TIME(7, ABSOLUTE, TB_WORKSTATION_TIME, N+4, 0); seqScheduler.ENABLE_ROW(7); wait 10 s; seqScheduler.DISABLE_ROW(7); bracket (2 sched samples); MARK(35).
Window:    Marker(id=34) … Marker(id=35)
Pass rule: named checks declared in the integration test (test_L2_7). metrics: t(event seqScheduler.RowDispatched(index=7)) - t(seqScheduler.ENABLE_ROW sent) <= 6 s.
Evidence:  EVR RowDispatched(7) → SEQ_A → RowCompleted(7); DISABLE_ROW(7) at the end restores the default-like state for L2-6/L5-1.
Planted faults:none

ID:        L2-8
Title:     Time-base-relative row under a second base
Purpose:   Proves a TIME_BASE_RELATIVE row for TB_PROC_TIME waits for that base and dispatches the offset after it is first reported.
Covers:    REQ-SEQSCHED-009, REQ-SEQSCHED-010, REQ-SEQSCHED-011, REQ-SEQSCHED-014
Setup:     Row 6 disabled (re-armed by the edits); no override.
Procedure: MARK(36); seqScheduler.SET_ROW_SEQUENCE(6, "seqs/sched_b.bin"); seqScheduler.SET_ROW_TIME(6, TIME_BASE_RELATIVE, TB_PROC_TIME, 3, 0); seqScheduler.SET_ROW_FLAGS(6, false, false); seqScheduler.ENABLE_ROW(6); wait 5 s; MARK(37); timeBaseOverride.SET_TIME_BASE(TB_PROC_TIME); wait 8 s; timeBaseOverride.CLEAR_OVERRIDE; bracket (2 sched samples); MARK(38).
Window:    W1 = Marker(id=36) … Marker(id=37); W2 = Marker(id=37) … Marker(id=38)
Pass rule: named checks declared in the integration test (test_L2_8). metrics: t(event seqScheduler.RowDispatched(index=6)) - t(timeBaseOverride.SET_TIME_BASE sent) <= 6 s.
Evidence:  W1: no dispatch for 5 s; W2: TimeBaseChanged → RowDispatched(6) ≈3 s later → SEQ_B → RowCompleted(6).
Planted faults:none

ID:        L3-1
Title:     Persistence and start-relative across restart
Purpose:   Proves a saved table survives a restart and a START_RELATIVE row dispatches the offset after the new start.
Covers:    REQ-SEQSCHED-002, REQ-SEQSCHED-006, REQ-SEQSCHED-007, REQ-SEQSCHED-016, REQ-SEQSCHED-027, REQ-SEQSCHED-032, REQ-SEQSCHED-033
Setup:     Rows 0..7 in their post-L2 states; /tmp/PrmDb.dat writable (int_config.json Svc.PrmDb.filename).
Procedure: MARK(40); seqScheduler.DISABLE_ROW(1); seqScheduler.DISABLE_ROW(2); seqScheduler.DISABLE_ROW(3); seqScheduler.DISABLE_ROW(4); seqScheduler.DISABLE_ROW(5); seqScheduler.DISABLE_ROW(6); seqScheduler.SET_ROW_SEQUENCE(0, "seqs/sched_a.bin"); seqScheduler.SET_ROW_TIME(0, START_RELATIVE, TB_NONE, 5, 0); seqScheduler.ENABLE_ROW(0); seqScheduler.SAVE_TABLE; FileHandling.prmDb.PRM_SAVE_FILE; seqScheduler.SET_ROW_SEQUENCE(7, "seqs/sched_b.bin")  (unsaved edit, REQ-027); wait 2 s; MARK(41); action: restart Ref (bench reset template); wait until CdhCore.version ProjectVersion EVR after restart; MARK(42); wait 12 s; seqScheduler.DUMP_TABLE; bracket (2 sched samples); MARK(43).
Window:    W1 = Marker(id=40) … Marker(id=41); R = Marker(id=41) … Marker(id=42); W2 = Marker(id=42) … Marker(id=43)
Pass rule: named checks declared in the integration test (test_L3_1).
Evidence:  W1: TableSaved → PrmFileSaveComplete; R: FSW clock restart; W2: RowDispatched(0) within ≈5 s of first run cycle → SEQ_A → RowCompleted(0).
Planted faults:persist_late_row

ID:        L4-1
Title:     Relative clocks under a time base change
Purpose:   Proves SinceStart keeps accumulating and SinceBase restarts when the base changes, both advancing one second per run cycle over a 60 s strip.
Covers:    REQ-SEQSCHED-007, REQ-SEQSCHED-008, REQ-SEQSCHED-012, REQ-SEQSCHED-036, REQ-TBO-002, REQ-TBO-003, REQ-TBO-004
Setup:     No override; no enabled pending row due in the window; B = last(SinceBase), S = last(SinceStart) before the window.
Procedure: MARK(50); bracket (2 sched samples); timeBaseOverride.SET_TIME_BASE(TB_SC_TIME); wait 60 s; bracket (2 sched samples); timeBaseOverride.CLEAR_OVERRIDE; bracket (2 sched samples); MARK(51).
Window:    Marker(id=50) … Marker(id=51)
Pass rule: named checks declared in the integration test (test_L4_1). metrics: sched samples >= 55 (1 Hz run period; allow 5 missed downlink frames in 60 s).
Evidence:  Telemetry strips SinceStart (unbroken staircase) and SinceBase (two restarts) beside OverrideChanged/TimeBaseChanged EVRs.
Planted faults:none

ID:        L5-1
Title:     Late row skipped
Purpose:   Proves a row first due more than LATE_LIMIT_SECONDS ago is skipped with a warning and never dispatched.
Covers:    REQ-SEQSCHED-017, REQ-SEQSCHED-018, REQ-SEQSCHED-037
Setup:     Row 7 disabled; LATE_LIMIT_SECONDS at default 10; N = host UNIX seconds.
Procedure: MARK(60); seqScheduler.SET_ROW_SEQUENCE(7, "seqs/sched_a.bin"); seqScheduler.SET_ROW_TIME(7, ABSOLUTE, TB_WORKSTATION_TIME, N-60, 0); seqScheduler.ENABLE_ROW(7); wait 5 s; bracket (2 sched samples); MARK(61).
Window:    Marker(id=60) … Marker(id=61)
Pass rule: named checks declared in the integration test (test_L5_1). metrics: t(event seqScheduler.RowSkippedLate(index=7)) - t(seqScheduler.ENABLE_ROW sent) <= 3 s.
Evidence:  EVR RowSkippedLate(7, due, now) once; no RowDispatched; RowsSkipped steps to 1.
Planted faults:fresh_absolute

ID:        L5-4
Title:     Late start-relative row
Purpose:   Proves a START_RELATIVE row whose offset is already more than LATE_LIMIT_SECONDS in the past is skipped, never dispatched.
Covers:    REQ-SEQSCHED-017, REQ-SEQSCHED-018
Setup:     Row 7 disabled; S = last(SinceStart) >= 70.
Procedure: MARK(66); seqScheduler.SET_ROW_SEQUENCE(7, "seqs/sched_a.bin"); seqScheduler.SET_ROW_TIME(7, START_RELATIVE, TB_NONE, S-60, 0); seqScheduler.ENABLE_ROW(7); wait 5 s; bracket (2 sched samples); MARK(67).
Window:    Marker(id=66) … Marker(id=67)
Pass rule: named checks declared in the integration test (test_L5_4). metrics: t(event seqScheduler.RowSkippedLate(index=7)) - t(seqScheduler.ENABLE_ROW sent) <= 3 s.
Evidence:  EVR RowSkippedLate(7) once; no dispatch.
Planted faults:none

ID:        L5-2
Title:     Completion timeout
Purpose:   Proves an unreported end times out after DONE_TIMEOUT_TICKS and the late sequencer report is ignored as foreign.
Covers:    REQ-SEQSCHED-024, REQ-SEQSCHED-025, REQ-SEQSCHED-039, REQ-SEQSCHED-037
Setup:     Row 3 disabled; S = last(SinceStart); sched_long.bin (8 s).
Procedure: MARK(62); seqScheduler.DONE_TIMEOUT_TICKS_PRM_SET(3); seqScheduler.SET_ROW_SEQUENCE(3, "seqs/sched_long.bin"); seqScheduler.SET_ROW_TIME(3, START_RELATIVE, TB_NONE, S+3, 0); seqScheduler.ENABLE_ROW(3); wait 16 s; seqScheduler.DONE_TIMEOUT_TICKS_PRM_SET(30); bracket (2 sched samples); MARK(63).
Window:    Marker(id=62) … Marker(id=63)
Pass rule: named checks declared in the integration test (test_L5_2). metrics: t(event seqScheduler.RowTimedOut(index=3)) - t(event seqScheduler.RowDispatched(index=3)) <= 5 s.
Evidence:  EVR RowDispatched(3) → RowTimedOut(3) ≈3 s later → SEQ_L_END/CS_SequenceComplete ≈8 s later with no RowCompleted.
Planted faults:none

ID:        L5-3
Title:     Time base never seen
Purpose:   Proves a row for a base not reported within BASE_WAIT_TICKS is skipped with a warning.
Covers:    REQ-SEQSCHED-019, REQ-SEQSCHED-039, REQ-SEQSCHED-037
Setup:     Row 2 disabled; Ref up longer than 5 run cycles; no override.
Procedure: MARK(64); seqScheduler.BASE_WAIT_TICKS_PRM_SET(5); seqScheduler.SET_ROW_SEQUENCE(2, "seqs/sched_a.bin"); seqScheduler.SET_ROW_TIME(2, TIME_BASE_RELATIVE, TB_PROC_TIME, 1, 0); seqScheduler.ENABLE_ROW(2); wait 5 s; seqScheduler.BASE_WAIT_TICKS_PRM_SET(0); bracket (2 sched samples); MARK(65).
Window:    Marker(id=64) … Marker(id=65)
Pass rule: named checks declared in the integration test (test_L5_3). metrics: t(event seqScheduler.RowSkippedNoBase(index=2, timeBase=TB_PROC_TIME)) - t(seqScheduler.ENABLE_ROW sent) <= 3 s.
Evidence:  EVR RowSkippedNoBase(2, TB_PROC_TIME) once; no RowDispatched.
Planted faults:none

ID:        L6-1
Title:     Rejected edits
Purpose:   Proves every Table 6 violation and the override of TB_DONT_CARE are rejected with a warning and change nothing.
Covers:    REQ-SEQSCHED-029, REQ-SEQSCHED-030, REQ-TBO-006, REQ-SEQSCHED-037
Setup:     Any table state with no row due; P = last(RowsPending) before the window.
Procedure: MARK(70); seqScheduler.ENABLE_ROW(8); seqScheduler.SET_ROW_TIME(1, ABSOLUTE, TB_WORKSTATION_TIME, 1, 1000000); seqScheduler.SET_ROW_SEQUENCE(1, ""); seqScheduler.SET_ROW_TIME(1, ABSOLUTE, TB_DONT_CARE, 1, 0); timeBaseOverride.SET_TIME_BASE(TB_DONT_CARE); bracket (2 sched samples); MARK(71).
Window:    Marker(id=70) … Marker(id=71)
Pass rule: named checks declared in the integration test (test_L6_1).
Evidence:  Five rejection EVRs, no RowEdited/RowDispatched/OverrideChanged; RowsPending flat.
Planted faults:none

ID:        L6-2
Title:     Rejections while a row runs
Purpose:   Proves a running row cannot be edited while other rows can, and that bad-index, Table 6 and TB_DONT_CARE requests are rejected.
Covers:    REQ-SEQSCHED-031, REQ-SEQSCHED-028, REQ-SEQSCHED-029, REQ-SEQSCHED-030, REQ-TBO-006, REQ-SEQSCHED-037
Setup:     Row 5 disabled; S = last(SinceStart).
Procedure: MARK(72); seqScheduler.SET_ROW_SEQUENCE(5, "seqs/sched_long.bin"); seqScheduler.SET_ROW_TIME(5, START_RELATIVE, TB_NONE, S+2, 0); seqScheduler.ENABLE_ROW(5); wait 5 s; seqScheduler.SET_ROW_FLAGS(5, true, false); seqScheduler.SET_ROW_FLAGS(6, false, false); seqScheduler.ENABLE_ROW(9); seqScheduler.SET_ROW_TIME(6, ABSOLUTE, TB_DONT_CARE, 1, 0); timeBaseOverride.SET_TIME_BASE(TB_DONT_CARE); wait 10 s; bracket (2 sched samples); MARK(73).
Window:    Marker(id=72) … Marker(id=73)
Pass rule: named checks declared in the integration test (test_L6_2).
Evidence:  EVR RowDispatched(5) → RowRejected(5, ROW_DISPATCHED), RowEdited(6), RowRejected(9, BAD_INDEX), RowRejected(6, BAD_TIME_BASE), InvalidTimeBase → RowCompleted(5).
Planted faults:none

ID:        L7-1
Title:     Final state
Purpose:   Leaves every row disabled and pending, no override, and a table dump the auditor decodes independently.
Covers:    REQ-SEQSCHED-028, REQ-SEQSCHED-032, REQ-SEQSCHED-034
Setup:     End of run.
Procedure: MARK(80); seqScheduler.DISABLE_ROW(0); seqScheduler.DISABLE_ROW(1); seqScheduler.DISABLE_ROW(2); seqScheduler.DISABLE_ROW(3); seqScheduler.DISABLE_ROW(4); seqScheduler.DISABLE_ROW(5); seqScheduler.DISABLE_ROW(6); seqScheduler.DISABLE_ROW(7); timeBaseOverride.CLEAR_OVERRIDE; seqScheduler.SAVE_TABLE; seqScheduler.DUMP_TABLE; wait 3 s; bracket (2 sched samples); MARK(81).
Window:    Marker(id=80) … Marker(id=81)
Pass rule: named checks declared in the integration test (test_L7_1).
Evidence:  Final telemetry strip and the DP file (decoded by the auditor from the run directory, not from scheduler EVRs).
Planted faults:none

## 4 Traceability
| REQ | Tests | Note |
|---|---|---|
| REQ-SEQSCHED-001 | waived: UT only | table size/fields: structural, UT |
| REQ-SEQSCHED-002 | L3-1 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-003 | waived: UT only | invalid parameter at init needs a corrupt PrmDb file; not planted in Ref — UT |
| REQ-SEQSCHED-004 | L2-2, L2-7 + UT |  |
| REQ-SEQSCHED-005 | L2-2, L2-7 + UT |  |
| REQ-SEQSCHED-006 | L2-1, L3-1 + UT |  |
| REQ-SEQSCHED-007 | L2-1, L3-1, L4-1 + UT |  |
| REQ-SEQSCHED-008 | L4-1 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-009 | L2-3, L2-8 + UT |  |
| REQ-SEQSCHED-010 | L2-3, L2-8 + UT |  |
| REQ-SEQSCHED-011 | L2-3, L2-8 + UT |  |
| REQ-SEQSCHED-012 | L4-1 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-013 | waived: UT only | clock steps not induced on the shared host; cycle-counted clocks (Q21) — UT |
| REQ-SEQSCHED-014 | L2-1, L2-2, L2-3, L2-7, L2-8 + UT |  |
| REQ-SEQSCHED-015 | L2-4 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-016 | L2-1, L3-1 + UT |  |
| REQ-SEQSCHED-017 | L5-1, L5-4 + UT |  |
| REQ-SEQSCHED-018 | L5-1, L5-4 + UT |  |
| REQ-SEQSCHED-019 | L5-3 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-020 | L2-4 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-021 | L2-5 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-022 | waived: UT only | both flags set: UT |
| REQ-SEQSCHED-023 | L2-1, L2-2, L2-4, L2-5, L2-7 + UT |  |
| REQ-SEQSCHED-024 | L5-2 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-025 | L5-2 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-026 | L2-1, L2-2 + UT |  |
| REQ-SEQSCHED-027 | L3-1 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-028 | L2-1, L6-2, L7-1 + UT |  |
| REQ-SEQSCHED-029 | L6-1, L6-2 + UT |  |
| REQ-SEQSCHED-030 | L6-1, L6-2 + UT |  |
| REQ-SEQSCHED-031 | L6-2 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-032 | L3-1, L7-1 + UT |  |
| REQ-SEQSCHED-033 | L3-1 + UT | single system test; UT is the second verification — waiver form put to the human (Q23) |
| REQ-SEQSCHED-034 | L2-6, L7-1 + UT |  |
| REQ-SEQSCHED-035 | waived: UT only | no DP buffer: not plantable in Ref's DataProducts configuration — UT |
| REQ-SEQSCHED-036 | L0-2, L2-1, L4-1 + UT |  |
| REQ-SEQSCHED-037 | L2-1, L2-2, L2-3, L2-4, L2-5, L2-6, L5-1, L5-2, L5-3, L6-1, L6-2 + UT |  |
| REQ-SEQSCHED-038 | waived: UT only | ROW_n PRM_SET at run time: UT (parameter set of a struct via GDS is a stage-2 feasibility item) |
| REQ-SEQSCHED-039 | L5-2, L5-3 + UT |  |
| REQ-SEQSCHED-040 | waived: UT only | two instances: Ref carries one — UT |
| REQ-SEQSCHED-041 | waived: UT only | busy row fails without dispatch — UT (no system test plants a concurrent due row on the nominal path) |
| REQ-TBO-001 | waived: UT only | pass-through equality: UT (L0-2/L2-2 rely on it) |
| REQ-TBO-002 | L2-3, L4-1 + UT |  |
| REQ-TBO-003 | L2-3, L4-1 + UT |  |
| REQ-TBO-004 | L2-3, L4-1 + UT |  |
| REQ-TBO-005 | L0-2, L2-3 + UT |  |
| REQ-TBO-006 | L6-1, L6-2 + UT |  |

## 5 Metrics
- L2-1: t(event seqScheduler.RowDispatched(index=0)) - t(seqScheduler.ENABLE_ROW sent) <= 8 s
- L2-2: t(event seqScheduler.RowDispatched(index=1)) - t(timeBaseOverride.SET_TIME_BASE sent) <= 3 s
- L2-3: t(event seqScheduler.RowDispatched(index=2)) - t(timeBaseOverride.SET_TIME_BASE sent) <= 8 s
- L2-4: t(event seqScheduler.RowDispatched(index=4)) - t(seqScheduler.ENABLE_ROW sent) <= 18 s
- L2-5: t(event seqScheduler.RowDispatched(index=6)) - t(event seqScheduler.RowCancelled(index=6, running=5)) <= 1 s
- L2-7: t(event seqScheduler.RowDispatched(index=7)) - t(seqScheduler.ENABLE_ROW sent) <= 6 s
- L2-8: t(event seqScheduler.RowDispatched(index=6)) - t(timeBaseOverride.SET_TIME_BASE sent) <= 6 s
- L4-1: sched samples >= 55
- L5-1: t(event seqScheduler.RowSkippedLate(index=7)) - t(seqScheduler.ENABLE_ROW sent) <= 3 s
- L5-4: t(event seqScheduler.RowSkippedLate(index=7)) - t(seqScheduler.ENABLE_ROW sent) <= 3 s
- L5-2: t(event seqScheduler.RowTimedOut(index=3)) - t(event seqScheduler.RowDispatched(index=3)) <= 5 s
- L5-3: t(event seqScheduler.RowSkippedNoBase(index=2, timeBase=TB_PROC_TIME)) - t(seqScheduler.ENABLE_ROW sent) <= 3 s

## 6 Fault menu
Generated from `planted_faults.py` planted fault declarations (`fth planted-faults-menu`): disturb_disable (uut-state; L2-3); drop_wait_flag (stimulus; L2-4); fresh_absolute (stimulus; L5-1); late_absolute (stimulus; L2-2); no_cancel_flag (stimulus; L2-5); persist_late_row (uut-state; L3-1); wrong_sequence_name (stimulus; L2-1).

## 7 Not covered
- REQ-SEQSCHED-035 (no DP buffer), -003 (bad parameter at init), -040 (two instances), -022 (both flags), -013 (clock steps are not induced on the shared host), -038 (struct parameter set from the ground: GDS feasibility to confirm at stage 2): unit tests only.
- Lateness of a `TIME_BASE_RELATIVE` row discovered after a base switch (Q19) is exercised only indirectly (L5-1 uses an ABSOLUTE row).
- REQ-SEQSCHED-013: with cycle-counted clocks (Q21) only a host clock step would exercise it; not induced on the shared host — unit test only.

## 8 Decision log
1. 2026-11-10 designer — deployment = Ref system deployment plus seqScheduler/timeBaseOverride (the human's request); no reduced deployment.
2. 2026-11-10 designer — marker = `CdhCore.cmdDisp.CMD_NO_OP_STRING(<id>)` → `NoOpStringReceived` text `Received a NO-OP string=<id>`; the harness sender passes marker ids as strings (shell-quoted `str(arg)`).
3. 2026-11-10 designer — short event/command names are the Ref dictionary names minus the `Ref.` prefix (`seqScheduler.X`, `cmdSeq.X`); subtopology instances keep their prefix (`CdhCore.cmdDisp.X`, `FileHandling.prmDb.X`, `DataProducts.dpWriter.X`).
4. 2026-11-10 designer — sequences compiled with `fprime-seqgen` default time base TB_DONT_CARE so they run under any override (CmdSequencer `CS_TimeBaseMismatch` is skipped for TB_DONT_CARE — confirm at stage 2).
5. 2026-11-10 designer — telemetry `SinceStart`/`SinceBase` added to the FPP at test design to make START_RELATIVE rows testable live (L2-1/L2-4/L2-5/L5-2/L6-2 read S from telemetry); the design reviewers are told of the delta in their open round.
6. 2026-11-10 designer — L3 reset = restart the Ref process (bench `reset` template); `/tmp/PrmDb.dat` persists across it.
7. 2026-11-10 designer — final-state evidence = the L7-1 DP file decoded by the auditor from the run directory (independent of scheduler EVRs); `final_state.accepted_evidence` set accordingly.
8. 2026-11-10 designer — max_count = 2; six planted faults over seven tests (≥ 2 × max_count); one `uut-state` fault (`disturb_disable`, `persist_late_row`).
10. 2026-11-10 designer — design review round 1 ripple: row states split (COMPLETE/FAILED/CANCELLED/TIMED_OUT/SKIPPED_*), telemetry RowsFailed/NextMode, events ForeignSeqDone/QueueFull, synchronous DP get, seqStartIn attribution; checks updated in L0-2, L2-5, L2-6, L5-2, L7-1; no test added or removed.
11. 2026-11-10 designer — parameter-set command mnemonics (`seqScheduler.<PARAM>_PRM_SET`) are the fprime dictionary convention; confirm from the generated dictionary at stage 2 (design-review r2 low-confidence note).
9. 2026-11-10 designer — redundancy (revised r1 of system-test review): knob-free second tests L2-7 (004/005/014/023), L2-8 (009/010/011), L5-4 (017/018) added; REQ-002/-015/-020/-021/-027/-033 keep a single system test with an explicit waiver note; the waiver form is put to the human (Q23).
12. 2026-11-10 designer — system-test review r1: STD completeness uses helper `commands_sent_minus_seq` (sequence-issued NO-OPs are dispatched by cmdDisp but not in commands.jsonl); OpCodeError moved from STD into ABS, with ABS-REJ for the negative controls; every test declares ABS/ABS-REJ; run-level DONE_TIMEOUT_TICKS = 30 set in L0-2; L3-1 restart window is reset:false (wall-clock FSW time, Q25); L2-6/L7-1 dump checks compare every field; L2-2 adds the base-mismatch phase (REQ-005); L3-1 adds an unsaved edit (REQ-027); REQ-013 waived UT only; metrics predicates are matched at unseal and listed in fails.
13. 2026-11-10 designer — design gap found by the system-test reviewer: a row without waitPrevious due while another row is DISPATCHED now FAILS without dispatch (REQ-SEQSCHED-041, RowFailed(index, BUSY)); drop_wait_flag's signature re-derived accordingly.
14. 2026-11-10 designer — system-test review r2: helper counts sequence NO-OPs by the SEQ_ message prefix (markers are numeric); REQ-SEQSCHED-041 narrowed to rows with neither flag (cancelPrevious takes precedence); L2-2 W1 shortened to 6 s for a ≥4 s lateness margin; disturb_disable/no_cancel_flag fails re-derived; redundancy waiver form remains the human's call (Q23).
15. 2026-11-10 designer — system-test review r3 (post-bound, gate note G7): L2-5 metric sign corrected and added to no_cancel_flag.fails; SDD Behavior flag order aligned with REQ-022 (cancel before wait).
