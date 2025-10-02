module Svc {
module FaultProtection {
    @ Translates FATALs to fault reports
    instance fatalToFault: FatalToFault base id FaultProtection.BASE_ID + 0x00001000

    @ Handles fault responses via sequence
    instance sequenceResponder: SequenceResponder base id FaultProtection.BASE_ID + 0x00002000

    @ Handles fault responses via sequence
    instance rebootResponder: RebootResponder base id FaultProtection.BASE_ID + 0x00003000

    @ Maps fault reports to fault responses
    instance faultManager: FaultManager base id FaultProtection.BASE_ID + 0x00004000 \
        queue size FaultProtection.QueueSizes.faultManager \
        stack size FaultProtection.StackSizes.faultManager \
        priority FaultProtection.Priorities.faultManager


    topology Subtopology {
        instance faultManager
        instance fatalToFault
        instance sequenceResponder
        instance rebootResponder

        connections Faults {
            fatalToFault.faultOut -> faultManager.reportIn

            faultManager.stepDispatchOut[FaultConfig.Port.SEQUENCE_RESPONDER_PORT] -> sequenceResponder.faultResponseDispatch
            faultManager.stepDispatchOut[FaultConfig.Port.REBOOT_RESPONDER_PORT] -> rebootResponder.faultResponseDispatch
            faultManager.stepCancelOut[FaultConfig.Port.SEQUENCE_RESPONDER_PORT] -> sequenceResponder.faultResponseCancel
            faultManager.stepCancelOut[FaultConfig.Port.REBOOT_RESPONDER_PORT] -> rebootResponder.faultResponseCancel
            sequenceResponder.faultResponseComplete -> faultManager.stepCompletionIn
            rebootResponder.faultResponseComplete -> faultManager.stepCompletionIn
        }
    }
} # FaultProtection
} # Svb