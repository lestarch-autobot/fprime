module Svc {

  @ Runs command sequences at scheduled times from a table sized at configuration
  active component SeqScheduler {

    # ----------------------------------------------------------------------
    # Types
    # ----------------------------------------------------------------------

    @ How the time of a row is interpreted
    enum TimeMode : U8 {
      ABSOLUTE = 0 @< System time in the row's time base
      START_RELATIVE = 1 @< Delay after the first schedule tick
      TIME_BASE_RELATIVE = 2 @< Delay after the first tick reporting the row's time base
    }

    @ State of a row
    enum RowState : U8 {
      PENDING = 0 @< Not yet run or skipped
      EXECUTED = 1 @< Sequence was run and is reported complete
      SKIPPED = 2 @< Passed without a run
      EXECUTING = 3 @< Sequence was run and is not yet reported complete
    }

    @ What a row does when it comes due while a sequence run by this component is running
    enum BusyPolicy : U8 {
      WAIT_FOR_RUNNING = 0 @< Run once the running sequence is reported complete
      CANCEL_RUNNING = 1 @< Cancel the running sequence, then run once it is reported complete
      SKIP_WHEN_RUNNING = 2 @< Become SKIPPED; the sequence is not run
    }

    @ One row of the table; a row with an empty file name is an empty row
    struct Row {
      $time: Fw.TimeValue @< Time or delay of the row
      mode: TimeMode @< Interpretation of time
      fileName: string size FileNameStringSize @< Sequence file
      busyPolicy: BusyPolicy @< Action when due while a sequence is running
      enabled: bool @< Row is run when due
      $state: RowState @< Row state; not held in the table file
    } default { enabled = false, busyPolicy = BusyPolicy.WAIT_FOR_RUNNING }

    @ Reason a table load failed
    enum LoadFailure : U8 {
      OPEN = 0 @< File cannot be opened, or does not exist on a load command
      READ = 1 @< File cannot be read
      SIZE = 2 @< File size is wrong
      VERSION = 3 @< File version is not supported
      CHECKSUM = 4 @< Checksum does not match
      ROW_CONTENT = 5 @< A row holds an invalid value
    }

    @ Reason a table save failed
    enum SaveFailure : U8 {
      OPEN = 0 @< File cannot be opened
      WRITE = 1 @< File cannot be written
      RENAME = 2 @< Written file cannot be renamed onto the table file
    }

    @ Why a row whose time passed was skipped as disabled
    enum SkipCause : U8 {
      ROW_DISABLED = 0 @< The row was disabled when its time passed
      SCHEDULER_DISABLED = 1 @< The component was disabled when the row's time passed
      WAITING_AT_DISABLE = 2 @< The row was waiting for a running sequence when the component was disabled
    }

    @ Reason a command was rejected
    enum CmdRejection : U8 {
      ROW_INDEX = 0 @< Row index out of range
      FILE_NAME = 1 @< Empty sequence file name, or enabling a row that has none
      NO_MATCH = 2 @< No row holds the sequence file name
      TIME = 3 @< Microseconds above 999999
      TABLE_EMPTY = 4 @< Save of a table whose rows are all empty
    }

    # ----------------------------------------------------------------------
    # General ports
    # ----------------------------------------------------------------------

    @ Schedule tick; counted on the caller's thread, at most one tick message is queued at a time
    sync input port run: Svc.Sched

    @ Handles the ticks counted since the last one handled
    internal port runInternal()

    @ Completion report from the attached sequencer
    async input port seqDoneIn: Fw.CmdResponse

    @ Query for an upcoming row; guarded: read on the caller's thread while the component thread edits the table
    guarded input port queryIn: Svc.SeqSchedulerQuery

    @ Runs a sequence on the attached sequencer
    output port seqRunOut: Svc.CmdSeqIn

    @ Health ping; a ping arriving on a full queue is dropped
    async input port pingIn: Svc.Ping drop

    @ Health ping reply
    output port pingOut: Svc.Ping

    @ Sequence cancel to the attached sequencer
    output port seqCancelOut: Svc.CmdSeqCancel

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Load the table from the table file
    async command LOAD_TABLE

    @ Write the table, without row states, to the table file
    async command SAVE_TABLE

    @ Enable or disable a row
    async command SET_ROW_ENABLED(
      row: FwSizeType @< Row index
      enabled: bool @< New enabled flag
    )

    @ Enable or disable every row holding a sequence file name
    async command SET_SEQ_ENABLED(
      fileName: string size FileNameStringSize @< Sequence file
      enabled: bool @< New enabled flag
    )

    @ Set a row; the row returns to PENDING
    async command SET_ROW(
      row: FwSizeType @< Row index
      $time: Fw.TimeValue @< Time or delay; microseconds 0 to 999999
      mode: TimeMode @< Interpretation of time
      fileName: string size FileNameStringSize @< Sequence file
      busyPolicy: BusyPolicy @< Action when due while a sequence is running
    )

    @ Produce a data product of the table
    async command DUMP_TABLE

    @ Report the next-due row in an event
    async command REPORT_ACTIVE_ROW

    @ Enable or disable the whole component; while disabled every row is treated as disabled
    async command SET_ENABLED(
      enabled: bool @< New enabled flag
    )

    # ----------------------------------------------------------------------
    # Parameters
    # ----------------------------------------------------------------------

    @ Table file path; when not set or empty, the default table file given to configure is used
    param TABLE_FILE: string size FileNameStringSize default ""

    @ Seconds past due within which a row is still run
    param LATE_TOLERANCE_SEC: U32 default 1


    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Table loaded
    event TableLoaded(fileName: string size FileNameStringSize, rows: FwSizeType) \
      severity activity high format "Loaded sequence schedule table {}: {} rows"

    @ Table file not found at the start-up load: the table is empty
    event TableFileMissing(fileName: string size FileNameStringSize) \
      severity warning low format "Sequence schedule table {} not found: table is empty"

    @ Table file holds more rows than the table; the rows that fit were loaded
    event TableFileRowsDropped(fileName: string size FileNameStringSize, fileRows: FwSizeType, tableRows: FwSizeType) \
      severity warning low format "Sequence schedule table {} holds {} rows: only {} loaded"

    @ Table load failed
    event TableLoadFailed(fileName: string size FileNameStringSize, reason: LoadFailure) \
      severity warning high format "Failed to load sequence schedule table {}: {}"

    @ Table saved
    event TableSaved(fileName: string size FileNameStringSize) \
      severity activity high format "Saved sequence schedule table {}"

    @ Table save failed
    event TableSaveFailed(fileName: string size FileNameStringSize, reason: SaveFailure) \
      severity warning high format "Failed to save sequence schedule table {}: {}"

    @ Sequence of a row run
    event SeqRun(row: FwSizeType, fileName: string size FileNameStringSize) \
      severity activity high format "Row {} ran sequence {}"

    @ Sequence of a row completed
    event SeqCompleted(row: FwSizeType, response: Fw.CmdResponse) \
      severity activity high format "Row {} sequence completed: {}"

    @ Cancel of a running sequence sent for a due CANCEL_RUNNING row
    event SeqCancelled(cancelledRow: FwSizeType, row: FwSizeType) \
      severity activity high format "Row {} sequence cancelled for row {}"

    @ Completion report received while no sequence run by this component is running
    event SeqDoneUnexpected(response: Fw.CmdResponse) \
      severity warning low format "Completion report with no sequence running: {}"

    @ SKIP_WHEN_RUNNING row came due while a sequence was running
    event RowSkippedBusy(row: FwSizeType, runningRow: FwSizeType) \
      severity warning low format "Row {} skipped: row {} sequence running"

    @ Due row skipped because a CANCEL_RUNNING row came due after it
    event RowSkippedByCancel(row: FwSizeType, cancelRow: FwSizeType) \
      severity warning low format "Row {} skipped: row {} due after it cancels"

    @ Row found late and skipped; not emitted on the first tick handled after a table load
    event RowLate(row: FwSizeType) \
      severity warning low format "Row {} skipped: found past its due time"

    @ Time of a disabled row passed; not emitted on the first tick handled after a table load
    event RowSkippedDisabled(row: FwSizeType, cause: SkipCause) \
      severity activity low format "Row {} skipped: {}"

    @ Rows were skipped on one tick or by one disable command beyond the rows that each got an event
    event RowSkipsNotReported(count: FwSizeType) \
      severity warning low format "{} more rows skipped without an event each"

    @ A reported time base was not recorded: SeqSchedulerTimeBases bases are already recorded
    @ Throttle is cleared at each successful load
    event TimeBaseNotRecorded(timeBase: TimeBase) \
      severity warning high format "Time base {} not recorded: record full" throttle 8


    @ Row enabled flag set
    event RowEnabledSet(row: FwSizeType, enabled: bool) \
      severity activity high format "Row {} enabled flag set to {}"

    @ Rows holding a sequence file enabled or disabled
    event SeqEnabledSet(fileName: string size FileNameStringSize, enabled: bool, rows: FwSizeType) \
      severity activity high format "Sequence {} enabled flag set to {} in {} rows"

    @ Row set
    event RowSet(row: FwSizeType, $time: Fw.TimeValue, mode: TimeMode, fileName: string size FileNameStringSize, busyPolicy: BusyPolicy) \
      severity activity high format "Row {} set: time {} mode {} sequence {} busy policy {}"

    @ Command rejected
    event CmdRejected(opCode: FwOpcodeType, reason: CmdRejection) \
      severity warning high format "Command {} rejected: {}"

    @ The next-due row, reported on command
    event ActiveRow(row: FwSizeType, timeDue: Fw.TimeValue, fileName: string size FileNameStringSize) \
      severity activity high format "Row {} is next, due at {}: sequence {}"

    @ No row is upcoming, reported on command
    event NoActiveRow() \
      severity activity high format "No row is upcoming"

    @ Component enabled or disabled
    event EnabledSet(enabled: bool) \
      severity activity high format "Sequence scheduler enabled flag set to {}"

    @ Table data product sent
    event TableDumped() \
      severity activity high format "Table data product sent"

    @ The product container filled before the last row; the rows that fit were sent
    event TableDumpTruncated(rowsSent: FwSizeType, rows: FwSizeType) \
      severity warning low format "Table data product holds {} of {} rows"

    @ No buffer granted for the table data product
    event TableDumpFailed() \
      severity warning high format "No buffer granted for the table data product"

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ Index of the soonest upcoming row; the table row count when none
    telemetry NextRow: FwSizeType

    @ Time due of that row
    telemetry NextTimeDue: Fw.TimeValue

    @ A table load has succeeded, with or without a file, and its table is in use; unchanged by a failed load command
    telemetry TableLoaded: bool

    @ The component is enabled
    telemetry Enabled: bool

    @ Row whose sequence was last reported complete; not written until there is one
    telemetry LastExecutedRow: FwSizeType

    @ Row whose sequence is running or being cancelled; the table row count when none
    telemetry RunningRow: FwSizeType

    @ Count of due rows waiting for the running sequence to be reported complete
    telemetry RowsWaiting: FwSizeType

    @ Count of PENDING rows that are not empty
    telemetry RowsPending: FwSizeType

    @ Count of EXECUTED rows
    telemetry RowsExecuted: FwSizeType

    @ Count of SKIPPED rows
    telemetry RowsSkipped: FwSizeType

    # ----------------------------------------------------------------------
    # Data products
    # ----------------------------------------------------------------------

    @ One row of the current table; one record per row, in index order
    product record RowRecord: Row id 0

    @ Container of the current table
    product container TableContainer id 0 default priority 10

    # ----------------------------------------------------------------------
    # Special ports
    # ----------------------------------------------------------------------

    time get port timeCaller
    command reg port cmdRegOut
    command recv port cmdIn
    command resp port cmdResponseOut
    text event port logTextOut
    event port logOut
    telemetry port tlmOut
    param get port prmGetOut
    param set port prmSetOut
    product get port productGetOut
    product send port productSendOut
  }
}
