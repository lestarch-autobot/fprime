// ======================================================================
// \title  SequenceResponder.cpp
// \author mstarch
// \brief  cpp file for SequenceResponder component implementation class
// ======================================================================

#include "Svc/FaultProtection/SequenceResponder/SequenceResponder.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

SequenceResponder ::SequenceResponder(const char* const compName) : SequenceResponderComponentBase(compName) {}

SequenceResponder ::~SequenceResponder() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

Fw::Success SequenceResponder ::responseDispatch_handler(FwIndexType portNum,
                                                         const FaultCfg::FaultResponse& faultResponse,
                                                         const FaultCfg::FaultId& faultId) {

    // TODO
    return Fw::Success::FAILURE;
}

}  // namespace Svc
