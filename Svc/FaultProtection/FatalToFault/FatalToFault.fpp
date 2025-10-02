module Svc {
module FaultProtection {
    @ Handles FATALs by translating into FaultReports
    passive component FatalToFault {
        import Svc.FaultProtection.Reporter

        @ FATAL event receive port
        sync input port FatalReceive: Svc.FatalEvent
    }
} # FaultProtection
} # Svc