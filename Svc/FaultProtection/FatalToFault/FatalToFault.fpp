module Svc {
    @ Handles FATALs by translating into FaultReports
    passive component FatalToFault {
        @ FATAL event receive port
        sync input port FatalReceive: Svc.FatalEvent

        @ Fault report port
        output port faultOut: Svc.FaultReport
    }
}