module FaultProtection {
    @ Translates FATALs to fault reports
    instance fatalToFault: Svc.FatalToFault base id FaultProtection.BASE_ID + 0x00001000

    @ Handles fault responses via sequence
    instance sequenceResponder: Svc.SequenceResponder base id FaultProtection.BASE_ID + 0x00002000

    @ Handles fault responses via sequence
    instance rebootResponder: Svc.RebootResponder base id FaultProtection.BASE_ID + 0x00003000

    @ Maps fault reports to fault responses
    instance faultManager: Svc.FaultManager base id FaultProtection.BASE_ID + 0x00004000 \
        queue size FaultProtection.QueueSizes.faultManager \
        stack size FaultProtection.StackSizes.faultManager \
        priority FaultProtection.Priorities.faultManager


    topology Subtopology {
        instance faultManager
        instance fatalToFault
        instance sequenceResponder

        connections Faults {
            fatalToFault.faultOut -> faultManager.reportIn

            faultManager.responseOut[0] -> sequenceResponder.responseDispatch
            faultManager.responseOut[1] -> sequenceResponder.responseDispatch
            faultManager.responseOut[2] -> rebootResponder.responseDispatch
            sequenceResponder.responseComplete -> faultManager.completionIn
            rebootResponder.responseComplete -> faultManager.completionIn
        }
    }
}