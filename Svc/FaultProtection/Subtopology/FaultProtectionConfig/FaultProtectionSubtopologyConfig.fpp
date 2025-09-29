module FaultProtection {

    constant BASE_ID = 0x0F000000

    module QueueSizes {
        constant faultManager  = 10
    }

    module StackSizes {
        constant faultManager  = 64 * 1024
    }

    module Priorities {
        constant faultManager  = 99
    }
}