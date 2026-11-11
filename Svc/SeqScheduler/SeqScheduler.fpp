module Svc {

  @ How a SeqScheduler row interprets its time
  enum SeqSchedulerMode : U8 {
    ABSOLUTE = 0 @< Row time is an absolute time in row timeBase
    START_RELATIVE = 1 @< Row time is an offset from the scheduler's first run cycle
    TIME_BASE_RELATIVE = 2 @< Row time is an offset from entry into row timeBase
  }

  @ Dispatch state of a SeqScheduler row
  enum SeqSchedulerRowState : U8 {
    PENDING = 0 @< Not yet dispatched (disabled rows stay here)
    DISPATCHED = 1 @< Sent to the sequencer; end not yet reported
    COMPLETE = 2 @< Sequencer reported OK
    FAILED = 3 @< Sequencer reported a non-OK end (rejection or failure)
    CANCELLED = 4 @< Cancelled by a cancelPrevious row
    TIMED_OUT = 5 @< No end reported within DONE_TIMEOUT_TICKS
    SKIPPED_LATE = 6 @< First due more than LATE_LIMIT_SECONDS ago
    SKIPPED_NO_BASE = 7 @< Time base not seen within BASE_WAIT_TICKS
  }

  @ One row of a SeqScheduler table (held as a parameter)
  struct SeqSchedulerRow {
    enabled: bool @< Row is considered for dispatch
    mode: SeqSchedulerMode @< Time interpretation
    timeBase: TimeBase @< Time base for ABSOLUTE and TIME_BASE_RELATIVE rows
    seconds: U32 @< Seconds of the time or offset
    useconds: U32 @< Microseconds of the time or offset
    waitPrevious: bool @< Hold while a scheduler-dispatched sequence runs
    cancelPrevious: bool @< Cancel a running scheduler-dispatched sequence first
    sequence: string size SeqSchedulerSequenceNameSize @< Sequence file name
  } default { enabled = false, mode = SeqSchedulerMode.START_RELATIVE, timeBase = TimeBase.TB_NONE }

  @ A row with its index and state, as written to the table data product
  struct SeqSchedulerRowRecord {
    index: U8
    rowState: SeqSchedulerRowState
    row: SeqSchedulerRow
  }

  @ Why a row command was rejected
  enum SeqSchedulerReject : U8 {
    BAD_INDEX = 0 @< Index outside the table
    BAD_USECONDS = 1 @< useconds >= 1000000
    EMPTY_SEQUENCE = 2 @< Empty sequence file name
    BAD_TIME_BASE = 3 @< TB_DONT_CARE or TB_NONE on a timed mode
    ROW_DISPATCHED = 4 @< Row has a running sequence
    PARAM_INVALID_INIT = 5 @< Enabled row parameter failed Table 6 at initialization
    PARAM_INVALID_SET = 6 @< Run-time ROW_n set failed Table 6; left unapplied
    PARAM_ROW_DISPATCHED = 7 @< Run-time ROW_n set targets a DISPATCHED row; left unapplied
  }

  @ Dispatches sequences from a parameter-backed table of timed rows
  queued component SeqScheduler {

    # ----------------------------------------------------------------------
    # General ports
    # ----------------------------------------------------------------------

    @ Run cycle: drains the queue, then evaluates the table
    sync input port run: Svc.Sched

    @ Starts a sequence on the sequencer
    output port seqRunOut: Svc.CmdSeqIn

    @ Cancels the running sequence on the sequencer
    output port seqCancelOut: Svc.CmdSeqCancel

    @ Sequence start reported by the sequencer (attributes the next seqDoneIn to the row)
    async input port seqStartIn: Svc.CmdSeqIn drop

    @ Sequence end (OK, rejection, failure or cancel) from the sequencer
    async input port seqDoneIn: Fw.CmdResponse drop

    @ Parameter-updated hook re-queued onto the run thread
    internal port paramUpdated(prmId: FwPrmIdType) drop

    # ----------------------------------------------------------------------
    # Special ports
    # ----------------------------------------------------------------------

    command recv port cmdIn
    command reg port cmdRegOut
    command resp port cmdResponseOut
    event port eventOut
    text event port textEventOut
    time get port timeGetOut
    telemetry port tlmOut
    param get port prmGetOut
    param set port prmSetOut
    product get port productGetOut
    product send port productSendOut

    # ----------------------------------------------------------------------
    # Data products
    # ----------------------------------------------------------------------

    @ One table row with its state
    product record RowRecord: SeqSchedulerRowRecord id 0

    @ The whole table: SeqSchedulerRows RowRecords
    product container Table id 0 default priority 10

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Enable a row (re-arms it)
    async command ENABLE_ROW(index: U8) opcode 0 hook

    @ Disable a row (re-arms it)
    async command DISABLE_ROW(index: U8) opcode 1 hook

    @ Replace a row's sequence file name (re-arms it)
    async command SET_ROW_SEQUENCE(index: U8, sequence: string size SeqSchedulerSequenceNameSize) opcode 2 hook

    @ Replace a row's mode, time base and time (re-arms it)
    async command SET_ROW_TIME(index: U8, mode: SeqSchedulerMode, timeBase: TimeBase, seconds: U32, useconds: U32) opcode 3 hook

    @ Replace a row's flags (re-arms it)
    async command SET_ROW_FLAGS(index: U8, waitPrevious: bool, cancelPrevious: bool) opcode 4 hook

    @ Write every row to its parameter
    async command SAVE_TABLE opcode 5 hook

    @ Produce the table data product
    async command DUMP_TABLE opcode 6 hook

    # ----------------------------------------------------------------------
    # Parameters
    # ----------------------------------------------------------------------

    @ Table row 0
    param ROW_0: SeqSchedulerRow id 0
    @ Table row 1
    param ROW_1: SeqSchedulerRow id 1
    @ Table row 2
    param ROW_2: SeqSchedulerRow id 2
    @ Table row 3
    param ROW_3: SeqSchedulerRow id 3
    @ Table row 4
    param ROW_4: SeqSchedulerRow id 4
    @ Table row 5
    param ROW_5: SeqSchedulerRow id 5
    @ Table row 6
    param ROW_6: SeqSchedulerRow id 6
    @ Table row 7
    param ROW_7: SeqSchedulerRow id 7

    @ A row first due more than this many seconds ago is skipped; 0 = never skip
    param LATE_LIMIT_SECONDS: U32 default 10 id 8

    @ Run cycles after start to wait for a row's time base; 0 = indefinitely
    param BASE_WAIT_TICKS: U32 default 0 id 9

    @ Run cycles to wait for a dispatched sequence's completion; 0 = no timeout
    param DONE_TIMEOUT_TICKS: U32 default 0 id 10

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ A row was sent to the sequencer
    event RowDispatched(index: U8, sequence: string size SeqSchedulerSequenceNameSize, dispatchTime: Fw.TimeValue) \
      severity activity high id 0 \
      format "Row {} dispatched sequence {} at {}"

    @ The sequencer reported OK for a dispatched row
    event RowCompleted(index: U8) severity activity high id 1 format "Row {} completed"

    @ The sequencer reported a non-OK end (rejection, failure or cancel) for a dispatched row
    event RowFailed(index: U8, response: Fw.CmdResponse) severity warning high id 2 \
      format "Row {} ended with response {}"

    @ No completion was reported within DONE_TIMEOUT_TICKS
    event RowTimedOut(index: U8) severity warning high id 3 format "Row {} completion timed out"

    @ A row was first due more than LATE_LIMIT_SECONDS ago
    event RowSkippedLate(index: U8, due: Fw.TimeValue, now: Fw.TimeValue) severity warning high id 4 \
      format "Row {} skipped: due {} found at {}"

    @ A row's time base was not seen within BASE_WAIT_TICKS
    event RowSkippedNoBase(index: U8, timeBase: TimeBase) severity warning high id 5 \
      format "Row {} skipped: time base {} never seen"

    @ A cancelPrevious row cancelled the running sequence
    event RowCancelled(index: U8, running: U8) severity activity high id 6 \
      format "Row {} cancelled running row {}"

    @ An edit command or run-time parameter changed a row
    event RowEdited(index: U8) severity activity low id 7 format "Row {} edited"

    @ A row command or run-time parameter was rejected
    event RowRejected(index: U8, reason: SeqSchedulerReject) severity warning low id 8 \
      format "Row {} rejected: {}"

    @ SAVE_TABLE wrote every row
    event TableSaved severity activity high id 9 format "Table saved to parameters"

    @ DUMP_TABLE could not get a data product buffer
    event DumpFailed(status: Fw.Success) severity warning low id 10 format "Table dump failed: {}"

    @ The system time base differs from the previous run cycle
    event TimeBaseChanged(previous: TimeBase, current: TimeBase) severity activity high id 11 \
      format "Time base changed from {} to {}"

    @ A sequence end arrived while no row was dispatched (ignored)
    event ForeignSeqDone(response: Fw.CmdResponse) severity diagnostic id 12 \
      format "Ignored sequence end {} not started by the scheduler"

    @ An OK end arrived before the dispatched row's start was reported (ignored as foreign)
    event UnconfirmedSeqDone(index: U8, response: Fw.CmdResponse) severity warning low id 14 \
      format "Row {} unconfirmed: ignored foreign sequence end {}"

    @ A command arrived with the queue full (answered BUSY)
    event QueueFull(op: FwOpcodeType) severity warning low id 13 \
      format "Queue full: command 0x{x} answered BUSY"

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ Index of the earliest due pending row; SeqSchedulerRows when none
    telemetry NextRow: U8 id 0

    @ Time or offset of the earliest due pending row; zero when none
    telemetry NextTime: Fw.TimeValue id 1

    @ Rows in PENDING
    telemetry RowsPending: U8 id 2

    @ Rows in DISPATCHED
    telemetry RowsDispatched: U8 id 3

    @ Rows in COMPLETE
    telemetry RowsComplete: U8 id 4

    @ Rows in SKIPPED_LATE or SKIPPED_NO_BASE
    telemetry RowsSkipped: U8 id 5

    @ Whole seconds since the scheduler's first run cycle (REQ-007/013)
    telemetry SinceStart: U32 id 6

    @ Whole seconds since the current system time base was entered (REQ-011/012)
    telemetry SinceBase: U32 id 7

    @ Mode of the NextRow row; NextTime is an offset unless ABSOLUTE
    telemetry NextMode: SeqSchedulerMode id 8

    @ Rows in FAILED, CANCELLED or TIMED_OUT
    telemetry RowsFailed: U8 id 9
  }
}
