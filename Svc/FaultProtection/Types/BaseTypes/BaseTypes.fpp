#* BaseTypes.fpp:
#*
#* Module containing the data types associated with FaultProtection that are intended for use within the FaultConfig
#* package. For convenience, they are added to the FaultConfig module.
module FaultConfig {
    @* Failure handling mode
    @*
    @* Defines the different options for handling failure statuses returned by a Step dispatch. Each Step shall be
    @* configured selecting how failures will be handled.
    enum FailureMode {
        IGNORE  @< IGNORE will ignore the error and continue with subsequent steps
        DEFER   @< DEFER will trigger FAULT_RESPONSE_FAILURE after all steps have run
        FAULT   @< FAULT will trigger FAULT_RESPONSE_FAILURE immediately
    }
}