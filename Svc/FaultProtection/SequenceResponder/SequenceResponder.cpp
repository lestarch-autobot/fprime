// ======================================================================
// \title  SequenceResponder.cpp
// \author mstarch
// \brief  cpp file for SequenceResponder component implementation class
// ======================================================================

#include "Svc/FaultProtection/SequenceResponder/SequenceResponder.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

SequenceResponder ::SequenceResponder(const char* const compName) : SequenceResponderComponentBase(compName) {}

SequenceResponder ::~SequenceResponder() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void SequenceResponder ::faultResponseCancel_handler(FwIndexType portNum) {
    // TODO
}

void SequenceResponder ::faultResponseDispatch_handler(FwIndexType portNum,
                                                       const FaultConfig::Response& response,
                                                       const FaultConfig::Step& step,
                                                       const FaultConfig::Context& context) {
    // TODO
}

}  // namespace FaultProtection

}  // namespace Svc
