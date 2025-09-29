module Svc {

    @ Port used to report a fault has occurred
    port FaultReport(faultId: FaultCfg.FaultId @< ID of the reported fault
    )

    @ Port used to dispatch a fault response
    port FaultResponseDispatch(
        faultResponse: FaultCfg.FaultResponse, @< Fault response enumeration
        faultId: FaultCfg.FaultId @< ID of the reported fault
    ) -> Fw.Success

    @ Port used to complete a fault response
    port FaultResponseComplete(
        status: Fw.Success, @< Status of the fault response
        faultResponse: FaultCfg.FaultResponse, @< Fault response enumeration
        faultId: FaultCfg.FaultId @< ID of the reported fault
    )
}
