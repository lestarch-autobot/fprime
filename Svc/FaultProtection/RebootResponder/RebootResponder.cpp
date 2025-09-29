// ======================================================================
// \title  RebootResponder.cpp
// \author mstarch
// \brief  cpp file for RebootResponder component implementation class
// ======================================================================

#include "Svc/FaultProtection/RebootResponder/RebootResponder.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

RebootResponder ::RebootResponder(const char* const compName) : RebootResponderComponentBase(compName) {}

RebootResponder ::~RebootResponder() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

Fw::Success RebootResponder ::responseDispatch_handler(FwIndexType portNum,
                                                       const FaultCfg::FaultResponse& faultResponse,
                                                       const FaultCfg::FaultId& faultId) {
    // TODO return
}

}  // namespace Svc
