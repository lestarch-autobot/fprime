module Svc {
module FaultProtection {

    @ Fault response table whose default value is the configuration fault response table
    array FaultResponseTable = [FaultConfig.Fault.NUM_FAULTS] FaultResponseEntry default FaultConfig.FaultResponseTable

    @ Response definition table whose default value is the configuration response definition table
    array ResponseDefinitionTable = [FaultConfig.Response.NUM_RESPONSES] ResponseDefinitionEntry default FaultConfig.ResponseDefinitionTable

    @ Step definition table whose default value is the configuration step definition table
    array StepDefinitionTable = [FaultConfig.Step.NUM_STEPS] StepDefinitionEntry default FaultConfig.StepDefinitionTable

    @ Enabled/disabled for each and every response
    array ResponsesEnabled = [FaultConfig.Response.NUM_RESPONSES] Fw.Enabled

    @ Failure mode of each step
    array StepFailureModes = [FaultConfig.Step.NUM_STEPS] FaultConfig.FailureMode


    @ Translates incoming Fault reports into outgoing fault response Steps
    active component FaultManager {

        @* Set a fault enabled state
        @*
        @* Enable/disable response to the supplied Fault ID. This will update the internal parameter and may be
        @* persisted by FAULT_RESPONSE_TABLE_SAVE. Command is dropped on overflow to prevent triggering fault response.
        async command SET_FAULT_ENABLED(fault: FaultConfig.Fault, enabled: Fw.Enabled) drop

        @* Set a response enabled state
        @*
        @* Enable/disable response. This will update the internal parameter and may be persisted by
        @* RESPONSE_TABLE_SAVE. Command is dropped on overflow to prevent triggering fault response.
        async command SET_RESPONSE_ENABLED(response: FaultConfig.Response, enabled: Fw.Enabled) drop
    
        @* Set a response step failure mode
        @*
        @* Set the FAILURE_MODE of response step. This will update the internal parameter and may be persisted by
        @* STEP_TABLE_SAVE. Command is dropped on overflow to prevent triggering fault response.
        async command UPDATE_STEP_FAILURE_MODE(step: FaultConfig.Step, failureMode: FaultConfig.FailureMode) drop

        @ Incoming fault report
        sync input port reportIn: FaultReport

        @ Outgoing response step dispatch
        output port stepDispatchOut: [FaultConfig.Port.NUM_PORTS] FaultResponseDispatch

        @ Outgoing response step cancel
        output port stepCancelOut: [FaultConfig.Port.NUM_PORTS] Fw.Signal

        @ Incoming response step completion
        async input port stepCompletionIn: FaultResponseComplete

        @ Internal port for handling non-discarded fault report
        internal port handleReport(fault: FaultConfig.Fault) # This fault must get through, or the FATAL system shall engage

        @ Event indicating a fault was reported
        event FaultReported(fault: FaultConfig.Fault) severity activity high format "Fault ID {} reported" throttle 5

        @ Event indicating a fault was reported and ignored due to higher-precedence active fault response
        event FaultIgnored(fault: FaultConfig.Fault) severity warning low format \
            "Fault ID {} reported and ignored due to higher-precedence active fault response" throttle 5

        @ Event indicating a fault was reported and ignored due to being disabled
        event FaultDisabled(fault: FaultConfig.Fault) severity warning low format \
            "Fault ID {} reported and disabled" throttle 5

        @ Fault response started
        event ResponseStarted(response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity high format "{} started, triggered by {}"

        @ Fault response completed
        event ResponseCompleted(response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity high format "{} completed, triggered by {}"
        
        @ Fault response step started
        event StepStarted(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity low format "{} started as part of {} triggered by {}"

        @ Fault response step completed
        event StepCompleted(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity low format "{} completed as part of {} triggered by {}"
        
        @ Unexpected fault response step completed
        event UnexpectedStepCompleted(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity warning high format "Unexpected {} completed as part of {} triggered by {}"

        @ Fault response step cancel requested
        event StepCancel(step: FaultConfig.Step) \
            severity activity high format "{} cancel requested"

        @ Fault response was disabled and thus skipped
        event StepSkipped(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity low format "{} disabled (skipped) as part of {} triggered by {}"

        @ Count of faults reported
        telemetry FaultsReported: FwSizeType

        @ Count of faults ignored
        telemetry FaultsIgnored: FwSizeType

        @ Fault response setting table
        external param FAULT_RESPONSE_TABLE: FaultResponseTable default FaultConfig.FaultResponseTable

        @ Response enabled table
        external param RESPONSE_TABLE: ResponsesEnabled

        @ Step failure mode table
        external param STEP_TABLE: StepFailureModes

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port for sending textual representation of events
        text event port logTextOut

        @ Port for sending events to downlink
        event port logOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port for retrieving parameters
        param get port prmGet

        @ Port for setting parameters
        param set port prmSet
    }

} # FaultProtection
} # Svc