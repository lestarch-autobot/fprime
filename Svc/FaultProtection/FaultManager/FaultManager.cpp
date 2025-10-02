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

FaultManager ::FaultManager(const char* const compName) : FaultManagerComponentBase(compName) {}

FaultManager ::~FaultManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FaultManager ::stepCompletionIn_handler(FwIndexType portNum,
                                         const Fw::Success& status,
                                         const FaultConfig::Response& response,
                                         const FaultConfig::Step& step) {
    // TODO
}

void FaultManager ::reportIn_handler(FwIndexType portNum, const FaultConfig::Fault& id) {
    // TODO
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void FaultManager ::SET_FAULT_ENABLED_cmdHandler(FwOpcodeType opCode,
                                                 U32 cmdSeq,
                                                 FaultConfig::Fault fault,
                                                 Fw::Enabled enabled) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FaultManager ::SET_RESPONSE_ENABLED_cmdHandler(FwOpcodeType opCode,
                                                    U32 cmdSeq,
                                                    FaultConfig::Step step,
                                                    Fw::Enabled enabled) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FaultManager ::UPDATE_RESPONSE_STEP_cmdHandler(FwOpcodeType opCode,
                                                    U32 cmdSeq,
                                                    FaultConfig::Step step,
                                                    FaultConfig::FailureMode failureMode) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined internal interfaces
// ----------------------------------------------------------------------

void FaultManager ::handleReport_internalInterfaceHandler(const FaultConfig::Fault& fault) {
    // TODO
}

Fw::SerializeStatus FaultManager ::deserializeParam(const FwPrmIdType base_id,
                                          const FwPrmIdType local_id,
                                          const Fw::ParamValid prmStat,
                                          Fw::SerializeBufferBase& buff) {
    return Fw::SerializeStatus::FW_SERIALIZE_NO_ROOM_LEFT;
}

Fw::SerializeStatus serializeParam(const FwPrmIdType base_id,
                                   const FwPrmIdType local_id,
                                   Fw::SerializeBufferBase& buff) {
    return Fw::SerializeStatus::FW_SERIALIZE_NO_ROOM_LEFT;
}

}  // namespace FaultProtection

}  // namespace Svc
