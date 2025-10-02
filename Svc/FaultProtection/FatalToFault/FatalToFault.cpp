// ======================================================================
// \title  FatalToFault.cpp
// \author mstarch
// \brief  cpp file for FatalToFault component implementation class
// ======================================================================

#include "Svc/FaultProtection/FatalToFault/FatalToFault.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FatalToFault ::FatalToFault(const char* const compName) : FatalToFaultComponentBase(compName) {}

FatalToFault ::~FatalToFault() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FatalToFault ::FatalReceive_handler(FwIndexType portNum, FwEventIdType Id) {
    // TODO
}

}  // namespace FaultProtection

}  // namespace Svc
