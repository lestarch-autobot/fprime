module Svc {
    @ Translates incoming FaultReports into outgoing FaultResponses
    active component FaultManager {

        @ Command to change handling of a given fault ID
        async command HANDLE_FAULT_ID(faultId: FaultCfg.FaultId, enable_disable: Fw.Enabled) drop

        @ Command to change handling of a given fault response
        async command HANDLE_FAULT_RESPONSE(faultResponse: FaultCfg.FaultResponse, enable_disable: Fw.Enabled) drop

        @ Port reporting a fault has occurred
        sync input port reportIn: Svc.FaultReport

        @ Port dispatching responses
        output port responseOut: [FaultCfg.FAULT_RESPONSE_OUT_PORTS] Svc.FaultResponseDispatch

        @ Port receiving completion status of responses
        async input port completionIn: Svc.FaultResponseComplete

        @ Internal port for handling non-discarded reports
        internal port handleReport(faultId: FaultCfg.FaultId) # This fault must get through, or the FATAL system shall engage

        @ Event indicating a fault was reported
        event FaultReported(faultId: FaultCfg.FaultId) severity activity high format "Fault ID {} reported"

        @ Event indicating a fault was reported and ignored due to active fault response
        event FaultIgnored(faultId: FaultCfg.FaultId) severity warning low format \
            "Fault ID {} reported and ignored due to active fault response"

        @ Event indicating a fault was reported and ignored due to active fault response
        event NoFaultResponse(faultId: FaultCfg.FaultId) severity warning high format \
            "Fault ID {} has no defined response"

        @ Fault response dispatched
        event FaultResponseDispatched(faultResponse: FaultCfg.FaultResponse, faultId: FaultCfg.FaultId) \
            severity activity high format "Fault Response {} dispatched in response to Fault ID {}"

        @ Fault response completed
        event FaultResponseCompleted(faultResponse: FaultCfg.FaultResponse, faultId: FaultCfg.FaultId) \
            severity activity high format "Fault Response {} completed in response to Fault ID {}"

        @ Fault response was disabled and thus skipped
        event FaultResponseSkipped(faultResponse: FaultCfg.FaultResponse, faultId: FaultCfg.FaultId) \
            severity activity high format "Fault Response {} disabled and skipped in response to Fault ID {}"
        

        @ Count of faults reported
        telemetry FaultsReported: FwSizeType

        @ Count of faults ignored
        telemetry FaultsIgnored: FwSizeType

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

    }
}