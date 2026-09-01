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

    @ State machine for handling fault response execution
    state machine FaultManagerStateMachine {
        @ Signal indicating a tick of the rate group
        signal Tick

        @ Signal indicating a step has been completed 
        signal StepSuccessful

        @ Signal indicating a step has failed
        signal StepFailed

        @ Signal indicating a step as completed, with a failure deferred until later
        signal StepDeferredFailure

        @ Check if there is a fault report
        guard hasReport

        @ Check if countdown has expired
        guard countdownExpired

        @ Check if response is done executing each step
        guard responseDone

        @ Action to start countdown
        action startCountdown

        @ Action to decrement countdown
        action decrementCountdown

        @ Action to select response to execute
        action selectResponse

        @ Action to select response to execute
        action completeResponse

        @ Action to dispatch a response step
        action dispatchStep

        @ Action to set response failure
        action setResponseFailure

        @ When a report is detected, enter COUNTDOWN to allow for additional reports to be processed before executing
        @ response otherwise return to the IDLE state to await the next tick and check again.
        choice CHECK_REPORT {
            if hasReport enter COUNTDOWN else enter IDLE
        }

        @ Check if the countdown allowing other reports to come in has expired. If so, dispatch the next step in a new
        @ response. If not, remain in COUNTDOWN by entering COUNTDOWN_ACTIVE, preventing restarting the countdown.
        choice CHECK_COUNTDOWN {
            if countdownExpired enter RESPONSE else enter COUNTDOWN.COUNTDOWN_ACTIVE
        }

        @ Check if the response is done executing all steps. If so return to the CHECK_REPORT check for new fault
        @ reports otherwise dispatch the next step in the response.
        choice CHECK_RESPONSE {
            if responseDone enter CHECK_REPORT else enter RESPONSE.DISPATCH_STEP
        }

        @ Enter IDLE state on initialization
        initial enter IDLE

        @ IDLE state: wait for fault report, warning on other signals
        state IDLE {
            on Tick enter CHECK_REPORT
        }

        @ COUNTDOWN state: wait for the countdown to expire allowing reports to accumulate
        state COUNTDOWN {
            initial enter COUNTDOWN_ACTIVE
            @ Reset the countdown on enter
            entry do { startCountdown }

            @ COUNTDOWN_ACTIVE state: wait for countdown to expire without resetting countdown
            state COUNTDOWN_ACTIVE {
                @ Decrement then check the countdown
                on Tick do { decrementCountdown } enter CHECK_COUNTDOWN
            }
        }

        @ RESPONSE state: process a series of response steps
        state RESPONSE {
            initial enter DISPATCH_STEP
            @ When entering the RESPONSE state, select the response to execute
            entry do { selectResponse }

            @ When leaving the RESPONSE state, complete the response
            exit do { completeResponse }

            @ DISPATCH_STEP state: dispatch a response step
            state DISPATCH_STEP {
                entry do { dispatchStep }

                @ When a step succeeds, check if the response is done
                on StepSuccessful enter CHECK_RESPONSE

                @ When a step fails, set response failure and return to CHECK_REPORT for new reports
                on StepFailed do { setResponseFailure } enter CHECK_REPORT

                @ When a step defers failure, set the response failure, but continue with checking for more steps
                on StepDeferredFailure do { setResponseFailure } enter CHECK_RESPONSE
            }
        }
    }

    @ Translates incoming Fault reports into outgoing fault response Steps
    active component FaultManager {
        @ Instantiate the FaultManagerStateMachine as the primary implementation mechanism for the FaultManager
        state machine instance faultManagerStateMachine: FaultManagerStateMachine

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