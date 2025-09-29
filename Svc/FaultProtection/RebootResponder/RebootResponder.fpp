module Svc {
    @ Reboot the FSW in response to a fault
    passive component RebootResponder {
        @ Incoming fault response dispatches
        sync input port responseDispatch: Svc.FaultResponseDispatch

        @ Outgoing fault response completion notifications (won't trigger, reboot)
        output port responseComplete: Svc.FaultResponseComplete
    }
}