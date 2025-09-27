module FaultCfg {
    constant FAULT_RESPONSE_OUT_PORTS = 3

    @ Enumeration (project configured) of the potential faults in the system
    enum FaultId {
        FAULT_RESPONSE_FAILURE @< REQUIRED: fault response reported by Svc.FaultManager
    }

    @ Enumeration (project configured) of the responses Svc.FaultManager can dispatch
    enum FaultResponse {
        SAMPLE_RESPONSE @< EXAMPLE: an example response
    }
}