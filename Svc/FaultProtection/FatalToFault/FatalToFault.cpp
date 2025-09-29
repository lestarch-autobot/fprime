// ======================================================================
// \title  FatalToFault.cpp
// \author mstarch
// \brief  cpp file for FatalToFault component implementation class
// ======================================================================

#include "Svc/FaultProtection/FatalToFault/FatalToFault.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FatalToFault ::FatalToFault(const char* const compName) : FatalToFaultComponentBase(compName) {}

FatalToFault ::~FatalToFault() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FatalToFault ::FatalReceive_handler(FwIndexType portNum, FwEventIdType Id) {
    this->faultOut_out(0, FaultCfg::FaultId::FATAL_OCCURRED);
}

}  // namespace Svc
