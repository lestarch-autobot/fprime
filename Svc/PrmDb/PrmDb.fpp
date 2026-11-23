module Svc {

  @ A component for storing parameters
  active component PrmDb {

    # ----------------------------------------------------------------------
    # Types
    # ----------------------------------------------------------------------

    @ Parameter DB type
    enum PrmDbType : U8 {
      DB_ACTIVE,
      DB_STAGING
    }

    @ State of parameter DB file load operations
    enum PrmDbFileLoadState : U8 {
        IDLE,
        LOADING_FILE_UPDATES,
        FILE_UPDATES_STAGED,
    }


    @ Outcome of the most recent parameter file load
    enum PrmFileLoadStatus : U8 {
      FAILED @< The load failed and no record from the file was kept
      PARTIAL @< The load failed after records were loaded into the active database
      LOADED @< Every record in the file was loaded
    }

    @ Parameter read error
    enum PrmReadError : U8 {
      OPEN
      DELIMITER
      DELIMITER_SIZE
      DELIMITER_VALUE
      RECORD_SIZE
      RECORD_SIZE_SIZE
      RECORD_SIZE_VALUE
      PARAMETER_ID
      PARAMETER_ID_SIZE
      PARAMETER_VALUE
      PARAMETER_VALUE_SIZE
      CRC
      CRC_SIZE
      CRC_BUFFER
      SEEK_ZERO
    }

    @ Parameter write error
    enum PrmWriteError : U8 {
      OPEN
      DELIMITER
      DELIMITER_SIZE
      RECORD_SIZE
      RECORD_SIZE_SIZE
      PARAMETER_ID
      PARAMETER_ID_SIZE
      PARAMETER_VALUE
      PARAMETER_VALUE_SIZE
      CRC_PLACE
      CRC_REAL
      CURR_POSITION
      SEEK_ZERO
      SEEK_POSITION
    }

    # ----------------------------------------------------------------------
    # General ports
    # ----------------------------------------------------------------------

    @ Port to get parameter values
    guarded input port getPrm: Fw.PrmGet

    @ Port to update parameters
    async input port setPrm: Fw.PrmSet

    @ Ping input port
    async input port pingIn: Svc.Ping drop

    @ Ping output port
    output port pingOut: Svc.Ping

    # ----------------------------------------------------------------------
    # Special ports
    # ----------------------------------------------------------------------

    @ Command receive port
    command recv port CmdDisp

    @ Command registration port
    command reg port CmdReg

    @ Command response port
    command resp port CmdStatus

    @ Event port
    event port Log

    @ Text event port
    text event port LogText

    @ Time get port
    time get port Time

    @ Telemetry port
    telemetry port Tlm

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    include "PrmDbCmdDict.fppi"

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    include "PrmDbEventDict.fppi"

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ Outcome of the most recent parameter file load
    telemetry PrmFileLoadStatus: PrmFileLoadStatus

    @ Number of records read from the file by the most recent parameter file load
    telemetry PrmFileLoadRecords: U32

  }

}
