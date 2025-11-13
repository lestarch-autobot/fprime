// ======================================================================
// \title  FaultManager.cpp
// \author mstarch
// \brief  cpp file for FaultManager component implementation class
// ======================================================================

#include "Svc/FaultProtection/FaultManager/FaultManager.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FaultManager ::FaultManager(const char* const compName) {
    this->registerExternalParameters(this);
}

FaultManager ::~FaultManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FaultManager ::reportIn_handler(FwIndexType portNum, const FaultConfig::Fault& id) {
    if (no fault) {
        startResponse();
    } else if (lower precedence) {
        cancelResponse();
    } else {
        ignoreResponse();
    }
}

void FaultManager ::stepCompletionIn_handler(FwIndexType portNum,
                                             const Fw::Success& status,
                                             const FaultConfig::Response& response,
                                             const FaultConfig::Step& step) {
    // TODO
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void FaultManager ::SET_FAULT_ENABLED_cmdHandler(FwOpcodeType opCode,
                                                 U32 cmdSeq,
                                                 FaultConfig::Fault fault,
                                                 Fw::Enabled enabled) {
    FW_ASSERT(fault < FaultConfig::Fault::NUM_FAULTS, static_cast<FwAssertArgType>(fault));
    this->m_fault_parameter[fault].set_enabled(enabled);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FaultManager ::SET_RESPONSE_ENABLED_cmdHandler(FwOpcodeType opCode,
                                                    U32 cmdSeq,
                                                    FaultConfig::Response response,
                                                    Fw::Enabled enabled) {
    FW_ASSERT(response < FaultConfig::Response::NUM_RESPONSES, static_cast<FwAssertArgType>(response));
    this->m_response_parameter[response] = enabled;
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FaultManager ::UPDATE_STEP_FAILURE_MODE_cmdHandler(FwOpcodeType opCode,
                                                        U32 cmdSeq,
                                                        FaultConfig::Step step,
                                                        FaultConfig::FailureMode failureMode) {
    FW_ASSERT(step < FaultConfig::Step::NUM_STEPS, static_cast<FwAssertArgType>(step));
    this->m_step_parameter[step] = failureMode;
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined internal interfaces
// ----------------------------------------------------------------------

void FaultManager ::handleReport_internalInterfaceHandler(const FaultConfig::Fault& fault) {
    // TODO
}

}  // namespace FaultProtection

}  // namespace Svc
