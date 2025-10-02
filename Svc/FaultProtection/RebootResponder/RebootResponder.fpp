module Svc {
module FaultProtection {
    @ Reboot the FSW in response to a fault
    passive component RebootResponder {
        import SyncResponder
    }
} # FaultProtection
} # Svc