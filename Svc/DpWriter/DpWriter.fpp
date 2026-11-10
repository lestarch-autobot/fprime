module Svc {

  @ A component for writing data products to disk
  active component DpWriter {

    # ----------------------------------------------------------------------
    # Scheduling ports
    # ----------------------------------------------------------------------

    @ Schedule in port
    sync input port schedIn: Svc.Sched

    @ Internal port that performs the schedIn work on the component thread
    internal port schedInInternal hook

    # ----------------------------------------------------------------------
    # Ports for handling data products
    # ----------------------------------------------------------------------

    @ Ports for receiving data products to write to disk
    async input port bufferSendIn: [DpWriterNumPorts] Fw.BufferSend

    @ Port for processing data products
    output port procBufferSendOut: [DpWriterNumProcPorts] Fw.BufferSend

    @ Ports for sending DpWritten notifications
    output port dpWrittenOut: [DpWriterNumPorts] DpWritten

    @ Ports for deallocating data product buffers
    output port deallocBufferSendOut: [DpWriterNumPorts] Fw.BufferSend

    # ----------------------------------------------------------------------
    # F' special ports
    # ----------------------------------------------------------------------

    @ Command receive port
    command recv port cmdIn

    @ Command registration port
    command reg port cmdRegIn

    @ Command response port
    command resp port cmdResponseOut

    @ Time get port
    time get port timeGetOut

    @ Telemetry port
    telemetry port tlmOut

    @ Event port
    event port eventOut

    @ Text event port
    text event port textEventOut

    # ----------------------------------------------------------------------
    # Commands 
    # ----------------------------------------------------------------------

    @ Clear event throttling
    async command CLEAR_EVENT_THROTTLE

    @ Set the processing override applied to every data product container
    async command SET_PROC_OVERRIDE(
                    $state: Fw.Enabled @< ENABLED forces the processing types on; DISABLED forces them off
                    procTypes: U8 @< Bit mask of Fw.DpCfg.ProcType values to force; zero clears the override
                  )

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Received buffer is invalid
    event InvalidBuffer \
      severity warning high \
      format "Received buffer is invalid" \
      throttle 10

    @ Received buffer is too small to hold a data product packet
    event BufferTooSmallForPacket(
                          bufferSize: FwSizeType @< The incoming buffer size
                          minSize: U32 @< The minimum required size
                        ) \
      severity warning high \
      format "Received buffer has size {}; minimum required size is {}" \
      throttle 10

    @ The received buffer has an invalid header hash
    event InvalidHeaderHash(
                             bufferSize: FwSizeType @< The incoming buffer size
                             storedHash: U32 @< The stored hash value
                             computedHash: U32 @< The computed hash value
                           ) \
      severity warning high \
      format "Received a buffer of size {} with an invalid header hash (stored {x}, computed {x})" \
      throttle 10

    @ Error occurred when deserializing the packet header
    event InvalidHeader(
                         bufferSize: FwSizeType @< The incoming buffer size
                         errorCode: U32 @< The error code
                       ) \
      severity warning high \
      format "Received buffer of size {}; deserialization of packet header failed with error code {}" \
      throttle 10

    @ Received buffer is too small to hold the data specified in the header
    event BufferTooSmallForData(
                          bufferSize: FwSizeType @< The incoming buffer size
                          minSize: U32 @< The minimum required size
                        ) \
      severity warning high \
      format "Received buffer has size {}; minimum required size is {}" \
      throttle 10

    @ An error occurred when formatting a file name
    event FileNameFormatError(
                         status: Fw.StringFormatStatus @< The status returned from the format operation
                       ) \
      severity warning high \
      format "Error {} formatting DP file name" \
      throttle 10

    @ An error occurred when opening a file
    event FileOpenError(
                         status: U32 @< The status code returned from the open operation
                         file: string size FileNameStringSize @< The file
                       ) \
      severity warning high \
      format "Error {} opening file {}" \
      throttle 10

    @ An error occurred when writing to a file
    event FileWriteError(
                          status: U32 @< The status code returned from the write operation
                          bytesWritten: U32 @< The number of bytes successfully written
                          bytesToWrite: U32 @< The number of bytes attempted
                          file: string size FileNameStringSize @< The file
                        ) \
      severity warning high \
      format "Error {} while writing {} of {} bytes to {}" \
      throttle 10

    @ The processing override was set
    event ProcOverrideSet(
                           $state: Fw.Enabled @< ENABLED forces the processing types on; DISABLED forces them off
                           procTypes: U8 @< The bit mask of processing types forced
                         ) \
      severity activity high \
      format "Processing override set: {} processing types {x}"

    @ The processing override was rejected
    event ProcOverrideInvalid(
                               $state: Fw.Enabled @< The requested state
                               procTypes: U8 @< The requested bit mask of processing types
                               allowed: U8 @< The bit mask of processing types accepted for the requested state
                             ) \
      severity warning low \
      format "Processing override rejected: {} processing types {x} exceed allowed {x}"

    @ File written
    event FileWritten(
                       bytes: U32 @< The number of bytes written
                       file: string size FileNameStringSize @< The file name
                     ) \
      severity activity low \
      format "Wrote {} bytes to file {}"

    # ----------------------------------------------------------------------
    # Telemetry
    # ----------------------------------------------------------------------

    @ The number of buffers received
    telemetry NumBuffersReceived: U32 update on change

    @ The number of bytes written
    telemetry NumBytesWritten: U64 update on change

    @ The number of successful writes
    telemetry NumSuccessfulWrites: U32 update on change

    @ The number of failed writes
    telemetry NumFailedWrites: U32 update on change

    @ The number of errors
    telemetry NumErrors: U32 update on change

    @ The processing override state: ENABLED forces processing types on; DISABLED forces them off
    telemetry ProcOverrideState: Fw.Enabled update on change

    @ The bit mask of processing types forced by the processing override; zero means no override
    telemetry ProcOverrideMask: U8 update on change

  }

}
