module Svc {
    @ Responds to faults via sequence
    passive component SequenceResponder {

        @ Incoming fault response dispatches
        sync input port responseDispatch: Svc.FaultResponseDispatch

        @ Outgoing fault response completion notifications
        output port responseComplete: Svc.FaultResponseComplete

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

    }
}