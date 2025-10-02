// ======================================================================
// \title  RebootResponder.cpp
// \author mstarch
// \brief  cpp file for RebootResponder component implementation class
// ======================================================================

#include "Svc/FaultProtection/RebootResponder/RebootResponder.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

RebootResponder ::RebootResponder(const char* const compName) : RebootResponderComponentBase(compName) {}

RebootResponder ::~RebootResponder() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void RebootResponder ::faultResponseCancel_handler(FwIndexType portNum) {
    // TODO
}

void RebootResponder ::faultResponseDispatch_handler(FwIndexType portNum,
                                                     const FaultConfig::Response& response,
                                                     const FaultConfig::Step& step,
                                                     const FaultConfig::Context& context) {
    // TODO
}

}  // namespace FaultProtection

}  // namespace Svc
