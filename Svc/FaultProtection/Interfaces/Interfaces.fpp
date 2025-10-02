module Svc {
module FaultProtection {

@* Reporter of a fault
@*
@* Use this interface on any component that needs to report a fault to FaultManager. It consists of the single output
@* port `faultOut` used to report the fault.
@*
@* Components using this interface can connect to FaultManager using: fault connections instance <instance name>.
interface Reporter {
    @ Report a fault to FaultManager
    output port faultOut: Svc.FaultProtection.FaultReport
}

@* Responder to a fault (asynchronous)
@*
@* Use this interface on any component that performs a fault response step asynchronously. It consists of an async
@* input port `faultResponseDispatch` used to accept the fault response step dispatch, an output port
@* `faultResponseComplete` used to respond after completing a fault response, and sync input port `faultResponseCancel`
@* used to implement the synchronous canceling of a fault response step.
interface AsyncResponder {
    @ Start a fault response step
    async input port faultResponseDispatch: FaultResponseDispatch

    @ Cancel a running fault response step
    sync input port faultResponseCancel: Fw.Signal

    @ Send fault response step completion
    output port faultResponseComplete: FaultResponseComplete
}

@* Responder to a fault (synchronous)
@*
@* Use this interface on any component that performs a fault response step asynchronously. It consists of an sync
@* input port `faultResponseDispatch` used to accept the fault response step dispatch, an output port
@* `faultResponseComplete` used to respond after completing a fault response, and sync input port `faultResponseCancel`
@* used to implement the synchronous canceling of a fault response step.
@*
@* [!CAUTION]
@* This interface was intended for the case when the user needs to use internal queuing, pass-through delegation, and
@* other tasks that rely on another component's queue. It SHALL NOT block the calling queue.
interface SyncResponder {
    @ Start a fault response step
    sync input port faultResponseDispatch: FaultResponseDispatch

    @ Cancel a running fault response step
    sync input port faultResponseCancel: Fw.Signal

    @ Send fault response step completion
    output port faultResponseComplete: FaultResponseComplete
}


} # FaultProtection
} # Svc